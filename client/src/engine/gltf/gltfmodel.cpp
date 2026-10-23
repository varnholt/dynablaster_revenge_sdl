#include "gltfmodel.h"

#include "framework/gldevice.h"
#include "tools/datapaths.h"

#include "logging.h"

#include "cgltf.h"
#include "stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>

namespace
{
using Matrix4 = std::array<float, 16>;

constexpr int32_t MAX_JOINTS = 16;

constexpr Matrix4 IDENTITY{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

// glTF's y-up to the game's z-up: (x, y, z) -> (x, -z, y)
constexpr Matrix4 Y_UP_TO_Z_UP{1, 0, 0, 0, 0, 0, 1, 0, 0, -1, 0, 0, 0, 0, 0, 1};

struct Shader
{
   uint32_t _program = 0;
   int32_t _model = -1;
   int32_t _joints = -1;
   int32_t _skinned = -1;
   int32_t _color = -1;
   int32_t _occlusion = -1;
   int32_t _flash = -1;
   int32_t _alpha = -1;
   int32_t _tint = -1;
};

Shader shader;

// column-major, a * b applies b first
Matrix4 multiply(const Matrix4& a, const Matrix4& b)
{
   Matrix4 result{};
   for (int32_t column = 0; column < 4; column++)
   {
      for (int32_t row = 0; row < 4; row++)
      {
         float sum = 0.0f;
         for (int32_t k = 0; k < 4; k++)
         {
            sum += a[static_cast<size_t>(k * 4 + row)] * b[static_cast<size_t>(column * 4 + k)];
         }
         result[static_cast<size_t>(column * 4 + row)] = sum;
      }
   }
   return result;
}

Matrix4 compose(const std::array<float, 3>& t, const std::array<float, 4>& q, const std::array<float, 3>& s)
{
   const float x = q[0];
   const float y = q[1];
   const float z = q[2];
   const float w = q[3];

   return {
      (1 - 2 * (y * y + z * z)) * s[0],
      (2 * (x * y + z * w)) * s[0],
      (2 * (x * z - y * w)) * s[0],
      0,
      (2 * (x * y - z * w)) * s[1],
      (1 - 2 * (x * x + z * z)) * s[1],
      (2 * (y * z + x * w)) * s[1],
      0,
      (2 * (x * z + y * w)) * s[2],
      (2 * (y * z - x * w)) * s[2],
      (1 - 2 * (x * x + y * y)) * s[2],
      0,
      t[0],
      t[1],
      t[2],
      1
   };
}

std::array<float, 4> slerp(const float* a, const float* b, float t)
{
   float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
   const float sign = dot < 0.0f ? -1.0f : 1.0f;
   dot *= sign;

   float wa = 1.0f - t;
   float wb = t * sign;

   if (dot < 0.9995f)
   {
      const float angle = std::acos(dot);
      const float sin_angle = std::sin(angle);
      wa = std::sin((1.0f - t) * angle) / sin_angle;
      wb = std::sin(t * angle) / sin_angle * sign;
   }

   std::array<float, 4> result{};
   float length = 0.0f;
   for (size_t i = 0; i < 4; i++)
   {
      result[i] = a[i] * wa + b[i] * wb;
      length += result[i] * result[i];
   }

   length = std::sqrt(length);
   for (float& value : result)
   {
      value /= length;
   }
   return result;
}

Matrix toEngine(const Matrix4& m)
{
   Matrix result;
   std::ranges::copy(m, result.values().begin());
   return result;
}

std::vector<float> readFloats(const cgltf_accessor* accessor)
{
   const size_t components = cgltf_num_components(accessor->type);
   std::vector<float> values(accessor->count * components);
   cgltf_accessor_unpack_floats(accessor, values.data(), values.size());
   return values;
}

void uploadTexture(uint32_t& texture, int32_t width, int32_t height, const std::vector<uint8_t>& pixels)
{
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
   glGenerateMipmap(GL_TEXTURE_2D);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

uint32_t whiteTexture()
{
   static uint32_t texture = 0;

   if (texture == 0)
   {
      uploadTexture(texture, 1, 1, {255, 255, 255, 255});
   }

   return texture;
}
}  // namespace

GltfModel::GltfModel(const std::string& filename)
{
   load(filename);
}

GltfModel::~GltfModel()
{
   for (Primitive& primitive : _primitives)
   {
      if (primitive._vertex_buffer)
      {
         glDeleteBuffers(1, &primitive._vertex_buffer);
         glDeleteBuffers(1, &primitive._index_buffer);
      }
   }

   for (Image& image : _images)
   {
      if (image._texture)
      {
         glDeleteTextures(1, &image._texture);
      }
   }
}

void GltfModel::load(const std::string& filename)
{
   std::ifstream file = DataPaths::open(filename);

   if (!file.is_open())
   {
      qWarning("GltfModel: %s not found", filename.c_str());
      return;
   }

   const std::vector<char> bytes{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

   cgltf_options options{};
   cgltf_data* data = nullptr;

   if (cgltf_parse(&options, bytes.data(), bytes.size(), &data) != cgltf_result_success ||
       cgltf_load_buffers(&options, data, nullptr) != cgltf_result_success)
   {
      qWarning("GltfModel: unable to read %s", filename.c_str());
      cgltf_free(data);
      return;
   }

   // nodes
   _nodes.resize(data->nodes_count);
   for (size_t i = 0; i < data->nodes_count; i++)
   {
      const cgltf_node& source = data->nodes[i];
      Node& node = _nodes[i];
      node._parent = source.parent ? static_cast<int32_t>(cgltf_node_index(data, source.parent)) : -1;

      if (source.has_translation)
      {
         std::copy_n(source.translation, 3, node._translation.begin());
      }
      if (source.has_rotation)
      {
         std::copy_n(source.rotation, 4, node._rotation.begin());
      }
      if (source.has_scale)
      {
         std::copy_n(source.scale, 3, node._scale.begin());
      }
   }

   // images, decoded once
   _images.resize(data->images_count);
   for (size_t i = 0; i < data->images_count; i++)
   {
      const cgltf_image& source = data->images[i];

      if (!source.buffer_view)
      {
         continue;
      }

      const auto* encoded = static_cast<const stbi_uc*>(cgltf_buffer_view_data(source.buffer_view));
      int32_t channels = 0;
      stbi_uc* pixels = stbi_load_from_memory(
         encoded, static_cast<int>(source.buffer_view->size), &_images[i]._width, &_images[i]._height, &channels, 4
      );

      if (pixels)
      {
         _images[i]._pixels.assign(pixels, pixels + static_cast<size_t>(_images[i]._width * _images[i]._height * 4));
         stbi_image_free(pixels);
      }
   }

   const auto image_of = [data](const cgltf_texture_view& view) -> int32_t
   { return view.texture && view.texture->image ? static_cast<int32_t>(cgltf_image_index(data, view.texture->image)) : -1; };

   // skin, at most one
   if (data->skins_count > 0)
   {
      const cgltf_skin& skin = data->skins[0];

      for (size_t j = 0; j < skin.joints_count && j < MAX_JOINTS; j++)
      {
         _joints.push_back(static_cast<int32_t>(cgltf_node_index(data, skin.joints[j])));
      }

      _inverse_bind.assign(_joints.size(), IDENTITY);
      if (skin.inverse_bind_matrices)
      {
         const std::vector<float> matrices = readFloats(skin.inverse_bind_matrices);
         for (size_t j = 0; j < _joints.size(); j++)
         {
            std::copy_n(matrices.begin() + static_cast<std::ptrdiff_t>(j * 16), 16, _inverse_bind[j].begin());
         }
      }
   }

   // meshes
   _minimum = Vector(1e9f, 1e9f, 1e9f);
   _maximum = Vector(-1e9f, -1e9f, -1e9f);

   for (size_t n = 0; n < data->nodes_count; n++)
   {
      const cgltf_node& node = data->nodes[n];

      if (!node.mesh)
      {
         continue;
      }

      for (size_t p = 0; p < node.mesh->primitives_count; p++)
      {
         const cgltf_primitive& source = node.mesh->primitives[p];

         if (source.type != cgltf_primitive_type_triangles)
         {
            continue;
         }

         Primitive primitive;
         primitive._node = static_cast<int32_t>(n);
         primitive._skinned = node.skin != nullptr && !_joints.empty();

         std::vector<float> positions;
         std::vector<float> normals;
         std::vector<float> uvs;
         std::vector<float> joints;
         std::vector<float> weights;

         for (size_t a = 0; a < source.attributes_count; a++)
         {
            const cgltf_attribute& attribute = source.attributes[a];

            switch (attribute.type)
            {
               case cgltf_attribute_type_position:
                  positions = readFloats(attribute.data);
                  break;
               case cgltf_attribute_type_normal:
                  normals = readFloats(attribute.data);
                  break;
               case cgltf_attribute_type_texcoord:
                  if (attribute.index == 0)
                  {
                     uvs = readFloats(attribute.data);
                  }
                  break;
               case cgltf_attribute_type_joints:
                  joints = readFloats(attribute.data);
                  break;
               case cgltf_attribute_type_weights:
                  weights = readFloats(attribute.data);
                  break;
               default:
                  break;
            }
         }

         const size_t count = positions.size() / 3;
         primitive._vertices.resize(count);

         for (size_t v = 0; v < count; v++)
         {
            Vertex& vertex = primitive._vertices[v];
            std::copy_n(positions.begin() + static_cast<std::ptrdiff_t>(v * 3), 3, vertex._position);

            if (normals.size() >= (v + 1) * 3)
            {
               std::copy_n(normals.begin() + static_cast<std::ptrdiff_t>(v * 3), 3, vertex._normal);
            }
            if (uvs.size() >= (v + 1) * 2)
            {
               std::copy_n(uvs.begin() + static_cast<std::ptrdiff_t>(v * 2), 2, vertex._uv);
            }

            std::fill_n(vertex._joints, 4, 0.0f);
            std::fill_n(vertex._weights, 4, 0.0f);
            vertex._weights[0] = 1.0f;

            if (joints.size() >= (v + 1) * 4 && weights.size() >= (v + 1) * 4)
            {
               std::copy_n(joints.begin() + static_cast<std::ptrdiff_t>(v * 4), 4, vertex._joints);
               std::copy_n(weights.begin() + static_cast<std::ptrdiff_t>(v * 4), 4, vertex._weights);
            }

            // bounds in game space, i.e. z-up
            const Vector z_up(vertex._position[0], -vertex._position[2], vertex._position[1]);
            _minimum = Vector(std::min(_minimum.x, z_up.x), std::min(_minimum.y, z_up.y), std::min(_minimum.z, z_up.z));
            _maximum = Vector(std::max(_maximum.x, z_up.x), std::max(_maximum.y, z_up.y), std::max(_maximum.z, z_up.z));
         }

         if (source.indices)
         {
            primitive._indices.resize(source.indices->count);
            for (size_t i = 0; i < source.indices->count; i++)
            {
               primitive._indices[i] = static_cast<uint32_t>(cgltf_accessor_read_index(source.indices, i));
            }
         }
         else
         {
            primitive._indices.resize(count);
            for (size_t i = 0; i < count; i++)
            {
               primitive._indices[i] = static_cast<uint32_t>(i);
            }
         }

         if (source.material)
         {
            primitive._color_image = image_of(source.material->pbr_metallic_roughness.base_color_texture);
            primitive._occlusion_image = image_of(source.material->occlusion_texture);
         }

         _primitives.push_back(std::move(primitive));
      }
   }

   // animations, sampled by the exporter so linear interpolation is enough
   for (size_t a = 0; a < data->animations_count; a++)
   {
      const cgltf_animation& source = data->animations[a];
      Animation animation;
      animation._name = source.name ? source.name : "";

      for (size_t c = 0; c < source.channels_count; c++)
      {
         const cgltf_animation_channel& channel = source.channels[c];

         if (!channel.target_node || !channel.sampler)
         {
            continue;
         }

         Channel target;
         target._node = static_cast<int32_t>(cgltf_node_index(data, channel.target_node));

         switch (channel.target_path)
         {
            case cgltf_animation_path_type_translation:
               target._path = 0;
               break;
            case cgltf_animation_path_type_rotation:
               target._path = 1;
               break;
            case cgltf_animation_path_type_scale:
               target._path = 2;
               break;
            default:
               continue;
         }

         target._step = channel.sampler->interpolation == cgltf_interpolation_type_step;
         target._times = readFloats(channel.sampler->input);
         target._values = readFloats(channel.sampler->output);

         if (!target._times.empty())
         {
            animation._duration = std::max(animation._duration, target._times.back());
         }

         animation._channels.push_back(std::move(target));
      }

      _animations.push_back(std::move(animation));
   }

   cgltf_free(data);
   _valid = !_primitives.empty();
}

bool GltfModel::isValid() const
{
   return _valid;
}

int32_t GltfModel::findAnimation(std::string_view name) const
{
   for (size_t i = 0; i < _animations.size(); i++)
   {
      if (_animations[i]._name == name)
      {
         return static_cast<int32_t>(i);
      }
   }

   return -1;
}

float GltfModel::getAnimationDuration(int32_t animation) const
{
   return animation >= 0 && animation < static_cast<int32_t>(_animations.size()) ? _animations[static_cast<size_t>(animation)]._duration
                                                                                   : 0.0f;
}

const Vector& GltfModel::getMinimum() const
{
   return _minimum;
}

const Vector& GltfModel::getMaximum() const
{
   return _maximum;
}

void GltfModel::initializeGL()
{
   if (_gl_initialized || !_valid)
   {
      return;
   }

   _gl_initialized = true;

   for (Image& image : _images)
   {
      if (!image._pixels.empty())
      {
         uploadTexture(image._texture, image._width, image._height, image._pixels);
         image._pixels.clear();
         image._pixels.shrink_to_fit();
      }
   }

   for (Primitive& primitive : _primitives)
   {
      glGenBuffers(1, &primitive._vertex_buffer);
      glBindBuffer(GL_ARRAY_BUFFER, primitive._vertex_buffer);
      glBufferData(
         GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(primitive._vertices.size() * sizeof(Vertex)), primitive._vertices.data(), GL_STATIC_DRAW
      );

      glGenBuffers(1, &primitive._index_buffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive._index_buffer);
      glBufferData(
         GL_ELEMENT_ARRAY_BUFFER,
         static_cast<GLsizeiptr>(primitive._indices.size() * sizeof(uint32_t)),
         primitive._indices.data(),
         GL_STATIC_DRAW
      );

      primitive._vertices.clear();
      primitive._vertices.shrink_to_fit();
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GltfModel::pose(int32_t animation, float time, std::vector<Matrix4>& globals) const
{
   std::vector<Node> nodes = _nodes;

   if (animation >= 0 && animation < static_cast<int32_t>(_animations.size()))
   {
      const Animation& clip = _animations[static_cast<size_t>(animation)];
      const float t = clip._duration > 0.0f ? std::fmod(time, clip._duration) : 0.0f;

      for (const Channel& channel : clip._channels)
      {
         if (channel._node < 0 || channel._times.empty())
         {
            continue;
         }

         // the key pair around t
         const auto upper = std::ranges::upper_bound(channel._times, t);
         const size_t next = std::min(static_cast<size_t>(std::distance(channel._times.begin(), upper)), channel._times.size() - 1);
         const size_t previous = next > 0 ? next - 1 : 0;
         const float span = channel._times[next] - channel._times[previous];
         const float blend = (span > 0.0f && !channel._step) ? std::clamp((t - channel._times[previous]) / span, 0.0f, 1.0f) : 0.0f;

         Node& node = nodes[static_cast<size_t>(channel._node)];
         const size_t width = channel._path == 1 ? 4 : 3;
         const float* a = channel._values.data() + previous * width;
         const float* b = channel._values.data() + next * width;

         if (channel._values.size() < (std::max(previous, next) + 1) * width)
         {
            continue;
         }

         if (channel._path == 1)
         {
            node._rotation = slerp(a, b, blend);
         }
         else
         {
            std::array<float, 3>& target = channel._path == 0 ? node._translation : node._scale;
            for (size_t i = 0; i < 3; i++)
            {
               target[i] = a[i] + (b[i] - a[i]) * blend;
            }
         }
      }
   }

   // parents before children isn't guaranteed, so resolve each chain
   globals.assign(nodes.size(), IDENTITY);
   std::vector<bool> done(nodes.size(), false);

   for (size_t i = 0; i < nodes.size(); i++)
   {
      std::vector<size_t> chain;
      for (auto current = static_cast<int32_t>(i); current >= 0 && !done[static_cast<size_t>(current)];
           current = nodes[static_cast<size_t>(current)]._parent)
      {
         chain.push_back(static_cast<size_t>(current));
      }

      for (auto it = chain.rbegin(); it != chain.rend(); ++it)
      {
         const Node& node = nodes[*it];
         const Matrix4 local = compose(node._translation, node._rotation, node._scale);
         globals[*it] = node._parent >= 0 ? multiply(globals[static_cast<size_t>(node._parent)], local) : local;
         done[*it] = true;
      }
   }
}

void GltfModel::begin()
{
   if (shader._program == 0)
   {
      shader._program = activeDevice().loadShader("gltf-vert.glsl", "gltf-frag.glsl");
      activeDevice().setShader(shader._program);
      shader._model = activeDevice().getParameterIndex("u_model");
      shader._joints = activeDevice().getParameterIndex("u_joints");
      shader._skinned = activeDevice().getParameterIndex("u_skinned");
      shader._color = activeDevice().getParameterIndex("u_color");
      shader._occlusion = activeDevice().getParameterIndex("u_occlusion");
      shader._flash = activeDevice().getParameterIndex("u_flash");
      shader._alpha = activeDevice().getParameterIndex("u_alpha");
      shader._tint = activeDevice().getParameterIndex("u_tint");
   }

   activeDevice().setShader(shader._program);
   activeDevice().bindSampler(shader._color, 0);
   activeDevice().bindSampler(shader._occlusion, 1);

   for (GLuint attribute = 0; attribute < 5; attribute++)
   {
      glEnableVertexAttribArray(attribute);
   }

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GltfModel::end()
{
   for (GLuint attribute = 0; attribute < 5; attribute++)
   {
      glDisableVertexAttribArray(attribute);
   }

   glDisable(GL_BLEND);
   glActiveTexture(GL_TEXTURE0);
   activeDevice().pop();
   activeDevice().setShader(0);
}

void GltfModel::draw(const Matrix& transform, int32_t animation, float time, const Look& look)
{
   if (!_valid)
   {
      return;
   }

   initializeGL();

   std::vector<Matrix4> globals;
   pose(animation, time, globals);

   std::array<Matrix, MAX_JOINTS> joint_matrices;
   for (size_t j = 0; j < _joints.size(); j++)
   {
      joint_matrices[j] = toEngine(multiply(globals[static_cast<size_t>(_joints[j])], _inverse_bind[j]));
   }

   activeDevice().setParameter(shader._flash, look._flash);
   activeDevice().setParameter(shader._alpha, look._alpha);
   activeDevice().setParameter(shader._tint, look._tint);

   for (const Primitive& primitive : _primitives)
   {
      // skinned vertices are posed by their joints, static ones by their node
      const Matrix4 local =
         primitive._skinned ? Y_UP_TO_Z_UP : multiply(Y_UP_TO_Z_UP, globals[static_cast<size_t>(primitive._node)]);
      const Matrix world = toEngine(local) * transform;

      activeDevice().push(world);
      activeDevice().setParameter(shader._model, world);
      activeDevice().setParameter(shader._skinned, primitive._skinned ? 1.0f : 0.0f);

      if (primitive._skinned)
      {
         activeDevice().setParameter(shader._joints, std::span<const Matrix>(joint_matrices.data(), MAX_JOINTS));
      }

      const auto texture_of = [this](int32_t image)
      { return image >= 0 && _images[static_cast<size_t>(image)]._texture ? _images[static_cast<size_t>(image)]._texture : whiteTexture(); };

      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, texture_of(primitive._occlusion_image));
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture_of(primitive._color_image));

      glBindBuffer(GL_ARRAY_BUFFER, primitive._vertex_buffer);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _position)));
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _normal)));
      glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _uv)));
      glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _joints)));
      glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, _weights)));

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, primitive._index_buffer);
      glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(primitive._indices.size()), GL_UNSIGNED_INT, nullptr);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}
