#include "gldevice.h"

#include "../gles3.h"
#include "../tools/datapaths.h"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <vector>

namespace
{
// a failed mapping yields an empty span
std::span<std::byte> mappedBytes(void* mapped, int32_t length)
{
   if (!mapped)
   {
      return {};
   }
   return {static_cast<std::byte*>(mapped), static_cast<size_t>(length)};
}

GLuint compileStage(GLenum type, std::span<const char> source, const std::string& filename)
{
   GLuint shader = glCreateShader(type);
   const GLchar* source_data = source.data();
   const auto length = static_cast<GLint>(source.size());
   glShaderSource(shader, 1, &source_data, &length);
   glCompileShader(shader);

   GLint status = GL_FALSE;
   glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
   if (status == GL_FALSE)
   {
      GLint log_length = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
      std::vector<char> log(static_cast<size_t>(log_length) + 1, '\0');
      glGetShaderInfoLog(shader, log_length, nullptr, log.data());
      SDL_Log("shader compile error in %s:\n%s", filename.c_str(), log.data());
      glDeleteShader(shader);
      return 0;
   }

   return shader;
}

// reads a whole shader source file and compiles it, 0 if missing or broken
GLuint loadStage(GLenum type, const std::string& filename)
{
   std::ifstream file = DataPaths::open(filename);
   if (!file.is_open())
   {
      SDL_Log("shader file not found: %s", filename.c_str());
      return 0;
   }

   const std::vector<char> source{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
   return compileStage(type, source, filename);
}

}  // namespace

bool GLDevice::init()
{
   _shader_alloc_index = 0;

   glClearColor(0.1f, 0.1f, 0.12f, 1.0f);

   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);
   glClearDepthf(1.0f);
   glClearStencil(0);

   glEnable(GL_CULL_FACE);

   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   // ANGLE's default window surface is sRGB-capable and auto-encodes every write to it,
   // double-gamma-brightening the already-sRGB art; GL_EXT_sRGB_write_control turns that off.
   // This is optional in GLES3, including on Switch. Only use the token when the
   // current context advertises it; otherwise glDisable produces GL_INVALID_ENUM.
   if (SDL_GL_ExtensionSupported("GL_EXT_sRGB_write_control"))
   {
      glDisable(GL_FRAMEBUFFER_SRGB_EXT);
   }

   _active = true;

   return true;
}

void GLDevice::resize(int32_t width, int32_t height)
{
   _width = width;
   _height = height;
   setViewPort(0, 0, width, height);
}

void GLDevice::setViewPort(int32_t x, int32_t y, int32_t width, int32_t height)
{
   _viewport_x = x;
   _viewport_y = y;
   _viewport_width = width;
   _viewport_height = height;

   glViewport(x, y, width, height);
}

RenderDevice::ViewPort GLDevice::getViewPort() const
{
   return {_viewport_x, _viewport_y, _viewport_width, _viewport_height};
}

void GLDevice::convertFromViewPort(int32_t& x, int32_t& y, int32_t target_width, int32_t target_height)
{
   x -= _viewport_x;
   y -= _viewport_y;
   x = (x * target_width) / _viewport_width;
   y = (y * target_height) / _viewport_height;
}

void GLDevice::clear()
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLDevice::clear(float r, float g, float b, float a)
{
   glClearColor(r, g, b, a);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
}

void GLDevice::setPerspective(float scale, float aspect, float z_near, float z_far)
{
   const float y_min = -z_near * scale;
   const float y_max = -y_min;
   const float x_max = y_max * aspect;
   const float x_min = y_min * aspect;

   _projection_matrix = Matrix::frustum(x_min, x_max, y_min, y_max, z_near, z_far);
}

void GLDevice::setCamera(const Matrix& matrix, float fov, float z_near, float z_far, bool perspective)
{
   if (perspective)
   {
      setPerspective(fov, static_cast<float>(_viewport_width) / _viewport_height, z_near, z_far);
   }
   else
   {
      const float x = 2.8f / fov;
      const float y = x * 9.0f / 16.0f;
      _projection_matrix = Matrix::ortho(-x, x, -y, y, z_near, z_far);
   }

   // the camera matrix goes into the projection slot while the world transform stays identity
   // until the next push(), so push()'s matrices are already relative to the camera
   _projection_matrix = matrix * _projection_matrix;
   _world_transform.identity();
}

void GLDevice::push(const Matrix& matrix)
{
   _world_transform = matrix;
   uploadTransformUniforms();
}

void GLDevice::pop()
{
   _world_transform.identity();
}

void GLDevice::uploadTransformUniforms()
{
   const auto shader = _shader_table.find(_current_shader);
   if (shader == _shader_table.end())
   {
      return;
   }

   const ShaderInfo& info = shader->second;

   if (info.model_view_location >= 0)
   {
      glUniformMatrix4fv(info.model_view_location, 1, GL_FALSE, _world_transform.values().data());
   }

   if (info.projection_location >= 0)
   {
      glUniformMatrix4fv(info.projection_location, 1, GL_FALSE, _projection_matrix.values().data());
   }

   if (info.model_view_projection_location >= 0)
   {
      const Matrix model_view_projection = _world_transform * _projection_matrix;
      glUniformMatrix4fv(info.model_view_projection_location, 1, GL_FALSE, model_view_projection.values().data());
   }

   if (info.normal_matrix_location >= 0)
   {
      // uploaded as mat4, shaders read the rotation part via mat3(u_normalMatrix)
      const Matrix normal_matrix = _world_transform.get3x3().adjointTranspose();
      glUniformMatrix4fv(info.normal_matrix_location, 1, GL_FALSE, normal_matrix.values().data());
   }
}

uint32_t GLDevice::createBuffer()
{
   uint32_t buffer = 0;
   glGenBuffers(1, &buffer);
   return buffer;
}

void GLDevice::deleteBuffer(uint32_t buffer)
{
   glDeleteBuffers(1, &buffer);
}

uint32_t GLDevice::createVertexBuffer(int32_t size, bool dynamic)
{
   const uint32_t buffer = createBuffer();
   allocateVertexBuffer(buffer, size, dynamic);
   return buffer;
}

void GLDevice::allocateVertexBuffer(uint32_t buffer, int32_t size, bool dynamic)
{
   glBindBuffer(GL_ARRAY_BUFFER, buffer);
   glBufferData(GL_ARRAY_BUFFER, size, nullptr, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
   _last_vertex_buffer_size = size;
}

std::span<std::byte> GLDevice::mapVertexBuffer(uint32_t buffer, int32_t size)
{
   glBindBuffer(GL_ARRAY_BUFFER, buffer);
   // WebGL2 rejects GL_MAP_WRITE_BIT alone; INVALIDATE_BUFFER matches the always-overwrite-everything usage
   const int32_t length = size != 0 ? size : _last_vertex_buffer_size;
   return mappedBytes(glMapBufferRange(GL_ARRAY_BUFFER, 0, length, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT), length);
}

void GLDevice::unlockVertexBuffer(uint32_t buffer)
{
   glBindBuffer(GL_ARRAY_BUFFER, buffer);
   glUnmapBuffer(GL_ARRAY_BUFFER);
}

uint32_t GLDevice::createIndexBuffer(int32_t size, bool dynamic)
{
   const uint32_t buffer = createBuffer();
   allocateIndexBuffer(buffer, size, dynamic);
   return buffer;
}

void GLDevice::allocateIndexBuffer(uint32_t buffer, int32_t size, bool dynamic)
{
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
   _last_index_buffer_size = size;
}

std::span<std::byte> GLDevice::mapIndexBuffer(uint32_t buffer, int32_t size)
{
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
   const int32_t length = size != 0 ? size : _last_index_buffer_size;
   return mappedBytes(glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, length, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT), length);
}

void GLDevice::unlockIndexBuffer(uint32_t buffer)
{
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
   glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
}

void GLDevice::setCulling(bool state)
{
   if (state)
   {
      glEnable(GL_CULL_FACE);
   }
   else
   {
      glDisable(GL_CULL_FACE);
   }
}

void GLDevice::setMaterial(const Vector&, const Vector&, const Vector&, float)
{
   // fixed-function per-vertex lighting (glMaterialfv) has no GLES equivalent
}

uint32_t GLDevice::createTexture(std::span<const uint32_t> data, int32_t x, int32_t y, int32_t flags)
{
   GLuint texture = 0;
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture);
   updateTexture(data, x, y, flags);
   return texture;
}

void GLDevice::deleteTexture(uint32_t texture_id)
{
   GLuint texture = texture_id;
   glDeleteTextures(1, &texture);
}

void GLDevice::updateTexture(std::span<const uint32_t> data, int32_t x, int32_t y, int32_t flags)
{
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (flags & 1) ? GL_LINEAR : GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & 2) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);

   if (flags & 4)
   {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   }
   else
   {
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   }

   int32_t level = 0;

   // asset data is laid out BGRA; GLES has no guaranteed BGRA format, so the channels are swapped
   // once up front, and the mip-chain averaging below (same-position byte lanes only) keeps them
   std::vector<uint32_t> pixels(static_cast<size_t>(x) * y);
   for (size_t i = 0; i < pixels.size(); ++i)
   {
      const uint32_t pixel = data[i];
      const uint32_t a = (pixel >> 24) & 0xff;
      const uint32_t r = (pixel >> 16) & 0xff;
      const uint32_t g = (pixel >> 8) & 0xff;
      const uint32_t b = pixel & 0xff;
      pixels[i] = (a << 24) | (b << 16) | (g << 8) | r;
   }

   do
   {
      if (x == 0)
      {
         x = 1;
      }
      if (y == 0)
      {
         y = 1;
      }

      glTexImage2D(GL_TEXTURE_2D, level, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

      const int32_t next_x = (x >> 1) == 0 ? 1 : (x >> 1);
      const int32_t next_y = (y >> 1) == 0 ? 1 : (y >> 1);

      size_t destination = 0;

      // a 1 pixel wide level has no second column to average with
      const int32_t column_step = (x > 1) ? 1 : 0;

      for (int32_t i = 0; i < next_y; ++i)
      {
         const size_t source1 = static_cast<size_t>(i) * 2 * x;
         const size_t source2 = (next_y > 1) ? source1 + x : source1;

         for (int32_t j = 0; j < next_x; ++j)
         {
            const uint32_t c1 = pixels[source1 + 2 * j];
            const uint32_t c2 = pixels[source1 + 2 * j + column_step];
            const uint32_t c3 = pixels[source2 + 2 * j];
            const uint32_t c4 = pixels[source2 + 2 * j + column_step];

            const uint32_t a = ((c1 >> 24 & 0xff) + (c2 >> 24 & 0xff) + (c3 >> 24 & 0xff) + (c4 >> 24 & 0xff)) >> 2;
            const uint32_t r = ((c1 >> 16 & 0xff) + (c2 >> 16 & 0xff) + (c3 >> 16 & 0xff) + (c4 >> 16 & 0xff)) >> 2;
            const uint32_t g = ((c1 >> 8 & 0xff) + (c2 >> 8 & 0xff) + (c3 >> 8 & 0xff) + (c4 >> 8 & 0xff)) >> 2;
            const uint32_t b = ((c1 & 0xff) + (c2 & 0xff) + (c3 & 0xff) + (c4 & 0xff)) >> 2;

            pixels[destination++] = (a << 24) | (r << 16) | (g << 8) | b;
         }
      }

      x >>= 1;
      y >>= 1;
      ++level;
   } while ((x != 0 || y != 0) && (flags & 2));
}

uint32_t GLDevice::loadShader(const std::string& vertex_name, const std::string& fragment_name)
{
   const GLuint vertex_shader = loadStage(GL_VERTEX_SHADER, vertex_name);
   const GLuint fragment_shader = loadStage(GL_FRAGMENT_SHADER, fragment_name);

   ShaderInfo info;
   info.program = glCreateProgram();

   if (vertex_shader != 0)
   {
      glAttachShader(info.program, vertex_shader);
   }
   if (fragment_shader != 0)
   {
      glAttachShader(info.program, fragment_shader);
   }

   glLinkProgram(info.program);

   GLint link_status = GL_FALSE;
   glGetProgramiv(info.program, GL_LINK_STATUS, &link_status);
   if (link_status == GL_FALSE)
   {
      GLint log_length = 0;
      glGetProgramiv(info.program, GL_INFO_LOG_LENGTH, &log_length);
      std::vector<char> log(static_cast<size_t>(log_length) + 1, '\0');
      glGetProgramInfoLog(info.program, log_length, nullptr, log.data());
      SDL_Log("shader link error (%s / %s):\n%s", vertex_name.c_str(), fragment_name.c_str(), log.data());
   }

   if (vertex_shader != 0)
   {
      glDeleteShader(vertex_shader);
   }
   if (fragment_shader != 0)
   {
      glDeleteShader(fragment_shader);
   }

   info.model_view_location = glGetUniformLocation(info.program, "u_modelView");
   info.projection_location = glGetUniformLocation(info.program, "u_projection");
   info.normal_matrix_location = glGetUniformLocation(info.program, "u_normalMatrix");
   info.model_view_projection_location = glGetUniformLocation(info.program, "u_modelViewProjection");

   const uint32_t shader = ++_shader_alloc_index;
   _shader_table[shader] = info;
   setShader(shader);

   return shader;
}

void GLDevice::setShader(uint32_t shader)
{
   if (_current_shader != shader)
   {
      _current_shader = shader;
      const auto info = _shader_table.find(shader);
      glUseProgram(info != _shader_table.end() ? info->second.program : 0);
   }
}

int32_t GLDevice::getParameterIndex(const std::string& name)
{
   const auto info = _shader_table.find(_current_shader);
   if (info == _shader_table.end())
   {
      return -1;
   }

   return glGetUniformLocation(info->second.program, name.c_str());
}

void GLDevice::bindSampler(int32_t position, int32_t unit)
{
   glUniform1i(position, unit);
}

void GLDevice::setParameter(int32_t position, std::span<const float> data)
{
   glUniform1fv(position, static_cast<GLsizei>(data.size()), data.data());
}

void GLDevice::setParameter(int32_t position, const Vector4& vector)
{
   glUniform4fv(position, 1, vector.values().data());
}

void GLDevice::setParameter(int32_t position, const Vector& vector)
{
   glUniform3fv(position, 1, vector.values().data());
}

void GLDevice::setParameter(int32_t position, const Vector2& vector)
{
   glUniform2fv(position, 1, vector.values().data());
}

void GLDevice::setParameter(int32_t position, const Matrix& matrix)
{
   glUniformMatrix4fv(position, 1, GL_FALSE, matrix.values().data());
}

void GLDevice::setParameter(int32_t position, std::span<const Matrix> matrices)
{
   glUniformMatrix4fv(position, static_cast<GLsizei>(matrices.size()), GL_FALSE, matrices.front().values().data());
}

void GLDevice::setParameter(int32_t position, float value)
{
   glUniform1f(position, value);
}

void GLDevice::setSwapInterval(int32_t interval)
{
   SDL_GL_SetSwapInterval(interval);
}
