// GLES3 port of client/src/game/deathflowfieldanimation.cpp - see project memory for the full
// GPGPU-particle-simulation background (this is a "render positions into a float texture, ping-
// pong update them via fragment shader passes, read them back into a VBO via glReadPixels" style
// particle simulation, not a straightforward material port). Every glBegin(GL_QUADS)/glVertex3f/
// glTexCoord2f fullscreen-quad pass becomes a small dynamic attribute-array draw (2 triangles),
// same treatment already established elsewhere in this port (e.g. MenuPageListItem::drawRows()).
// glMapBuffer (no GLES3 equivalent) becomes glMapBufferRange. Fixed-function client-state arrays
// in draw() (glEnableClientState/glVertexPointer/glTexCoordPointer) become glVertexAttribPointer/
// glEnableVertexAttribArray, matching the deathparticles-vert.glsl attribute layout (location 0 =
// position.xyz + alpha.w, location 1 = static per-particle UV).

#include "deathflowfieldanimation.h"

#include "framework/framebuffer.h"
#include "framework/gldevice.h"
#include "math/vector2.h"

#include <cmath>

// the number of particles actually used depends on the number of "empty" pixels which is typically 1/2.
#define PARTICLE_COUNT 8000

#define DISSOLVE_TIME 300

namespace
{
// one persistent VBO reused across every call (re-uploaded via glBufferData each time, not
// gen/delete'd) - this is called every frame for the ~5-second particle lifetime, and churning a
// fresh GL buffer object on every single call was a real performance/stability risk (buffer
// deletion can force an implicit GPU sync point on some drivers).
GLuint g_quad_vertex_buffer = 0;

void drawQuad(const float* verts, int floats_per_vertex)
{
   const int order[6] = {0, 1, 2, 0, 2, 3};
   float buffer[6 * 4];  // up to 4 floats/vertex (pos.xy + uv.xy), 6 verts

   for (int i = 0; i < 6; i++)
   {
      const float* src = verts + order[i] * floats_per_vertex;
      float* dst = buffer + i * floats_per_vertex;
      for (int c = 0; c < floats_per_vertex; c++)
         dst[c] = src[c];
   }

   if (g_quad_vertex_buffer == 0)
      glGenBuffers(1, &g_quad_vertex_buffer);

   glBindBuffer(GL_ARRAY_BUFFER, g_quad_vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * floats_per_vertex, buffer, GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * floats_per_vertex, (GLvoid*)0);

   if (floats_per_vertex > 2)
   {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * floats_per_vertex, (GLvoid*)(sizeof(float) * 2));
   }

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   if (floats_per_vertex > 2)
      glDisableVertexAttribArray(1);

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
}  // namespace

DeathFlowFieldAnimation::DeathFlowFieldAnimation() : _center(0.0f, 0.0f, 0.0f)
{
   _positions[0] = _positions[1] = 0;
   _target[0] = _target[1] = 0;
}

DeathFlowFieldAnimation::~DeathFlowFieldAnimation()
{
   glDeleteFramebuffers(1, &_target[0]);
   glDeleteFramebuffers(1, &_target[1]);
   glDeleteTextures(1, &_vertex_params);
   glDeleteTextures(1, &_positions[0]);
   glDeleteTextures(1, &_positions[1]);
   glDeleteBuffers(1, &_vertex_pos_buffer);
   glDeleteBuffers(1, &_vertex_uv_buffer);
   if (_texture)
      glDeleteTextures(1, &_texture);
}

const Vector& DeathFlowFieldAnimation::getCenter() const
{
   return _center;
}

void DeathFlowFieldAnimation::setCenter(const Vector& center)
{
   _center = center;
}

uint32_t DeathFlowFieldAnimation::getColorMap() const
{
   return _texture;
}

float DeathFlowFieldAnimation::getScale() const
{
   return _flow_scale;
}

void DeathFlowFieldAnimation::setScale(float scale)
{
   _flow_scale = scale;
}

float DeathFlowFieldAnimation::getPointSize() const
{
   return _particle_size;
}

bool DeathFlowFieldAnimation::isElapsed() const
{
   return _elapsed > DISSOLVE_TIME;
}

void DeathFlowFieldAnimation::initialize(FrameBuffer* src, const Vector& min, const Vector& max)
{
   // get 2d bounding rect (in pixels)
   int screen_width = src->width();
   int screen_height = src->height();

   int x0 = (int)std::floor((min.x + 1.0f) * 0.5f * screen_width);
   int y0 = (int)std::floor((min.y + 1.0f) * 0.5f * screen_height);
   int x1 = (int)std::ceil((max.x + 1.0f) * 0.5f * screen_width);
   int y1 = (int)std::ceil((max.y + 1.0f) * 0.5f * screen_height);

   int rect_width = x1 - x0;
   int rect_height = y1 - y0;

   // fix step size to approx. match number of particles
   float x_skip = std::sqrt((double)rect_width * rect_height / PARTICLE_COUNT);
   float y_skip = x_skip;

   _particle_size = 50.0f * x_skip;

   _width = (int)std::floor(rect_width / x_skip);
   _height = (int)std::floor(rect_height / y_skip);

   if (_width < 1)
      _width = 1;
   if (_height < 1)
      _height = 1;

   // copy color texture (rendering of player) - src is already bound as the current framebuffer
   // by PlayerDeathEffect::add() at this point.
   glGenTextures(1, &_texture);
   glBindTexture(GL_TEXTURE_2D, _texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x0, y0, rect_width, rect_height, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   // double buffer for vertex positions (ping/pong rendering)
   for (int i = 0; i < 2; i++)
   {
      glGenTextures(1, &_positions[i]);
      glBindTexture(GL_TEXTURE_2D, _positions[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, 0);
      glBindTexture(GL_TEXTURE_2D, 0);
   }

   // create double buffer framebuffers
   for (int i = 0; i < 2; i++)
   {
      glGenFramebuffers(1, &_target[i]);
      glBindFramebuffer(GL_FRAMEBUFFER, _target[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _positions[i], 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
   }

   // create vertexbuffer (filled later, per frame, via draw()'s glReadPixels)
   glGenBuffers(1, &_vertex_pos_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_pos_buffer);
   glBufferData(GL_ARRAY_BUFFER, _width * _height * sizeof(Vector4), 0, GL_DYNAMIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   // create vertex UV buffer (static) - one UV per particle, addressing the position/param textures
   glGenBuffers(1, &_vertex_uv_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_uv_buffer);
   const GLsizeiptr uv_buffer_size = (GLsizeiptr)_width * _height * sizeof(Vector2);
   glBufferData(GL_ARRAY_BUFFER, uv_buffer_size, 0, GL_DYNAMIC_DRAW);
   // WebGL2 rejects GL_MAP_WRITE_BIT alone (needs an INVALIDATE flag), valid on native GLES3 too.
   Vector2* dst2 = (Vector2*)glMapBufferRange(GL_ARRAY_BUFFER, 0, uv_buffer_size, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
   float dx = 1.0f / _width;
   for (int y = 0; y < _height; y++)
   {
      float fx = 0.0f;
      float fy = (float)y / _height;
      for (int x = 0; x < _width; x++)
      {
         dst2->x = fx;
         dst2->y = fy;
         dst2++;
         fx += dx;
      }
   }
   glUnmapBuffer(GL_ARRAY_BUFFER);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DeathFlowFieldAnimation::initializePositions(uint32_t depth_map, const Vector& min, const Vector& max)
{
   glBindFramebuffer(GL_FRAMEBUFFER, _target[1]);
   glViewport(0, 0, _width, _height);
   glBindTexture(GL_TEXTURE_2D, depth_map);

   const float quad[4 * 4] = {
      -1.0f,
      -1.0f,
      min.x,
      min.y,
      1.0f,
      -1.0f,
      max.x,
      min.y,
      1.0f,
      1.0f,
      max.x,
      max.y,
      -1.0f,
      1.0f,
      min.x,
      max.y,
   };
   drawQuad(quad, 4);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeathFlowFieldAnimation::initializeParams(uint32_t depth_map, const Vector& min, const Vector& max)
{
   uint32_t target = 0;

   glGenTextures(1, &_vertex_params);
   glBindTexture(GL_TEXTURE_2D, _vertex_params);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &target);
   glBindFramebuffer(GL_FRAMEBUFFER, target);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _vertex_params, 0);

   glViewport(0, 0, _width, _height);
   glBindTexture(GL_TEXTURE_2D, depth_map);

   const float quad[4 * 4] = {
      -1.0f,
      -1.0f,
      min.x,
      min.y,
      1.0f,
      -1.0f,
      max.x,
      min.y,
      1.0f,
      1.0f,
      max.x,
      max.y,
      -1.0f,
      1.0f,
      min.x,
      max.y,
   };
   drawQuad(quad, 4);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glDeleteFramebuffers(1, &target);
}

void DeathFlowFieldAnimation::update(float delta)
{
   _elapsed += delta;

   // move particles using double buffer: render to [_page] and read from [_page^1]
   glBindFramebuffer(GL_FRAMEBUFFER, _target[_page]);
   glViewport(0, 0, _width, _height);
   _page ^= 1;  // flip pages

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _positions[_page]);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, _vertex_params);

   glActiveTexture(GL_TEXTURE0);

   const float quad[4 * 2] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
   drawQuad(quad, 2);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DeathFlowFieldAnimation::draw()
{
   FrameBuffer* prev = FrameBuffer::Instance();

   // read the GPU-computed positions straight back into _vertex_pos_buffer - no transform feedback
   // in this GL version, so this manually does what transform feedback would.
   glBindFramebuffer(GL_FRAMEBUFFER, _target[1 - _page]);
   glViewport(0, 0, _width, _height);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, _vertex_pos_buffer);
   glReadPixels(0, 0, _width, _height, GL_RGBA, GL_FLOAT, 0);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

   if (prev)
   {
      glBindFramebuffer(GL_FRAMEBUFFER, prev->target());
      glViewport(0, 0, prev->width(), prev->height());
   }
   else
   {
      // no FrameBuffer currently pushed (this port has no MainDrawable owning a top-level FBO,
      // unlike the original desktop game where `prev` was never null here) - restore the
      // viewport to the real screen size, matching FrameBuffer::pop()'s own established
      // convention for this exact case, instead of leaving it stuck at _width x _height (the
      // tiny particle-texture size) for the rest of this frame and every frame after it.
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(activeDevice->getBorderLeft(), activeDevice->getBorderBottom(), activeDevice->getWidth(), activeDevice->getHeight());
   }

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_pos_buffer);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (GLvoid*)0);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_uv_buffer);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), (GLvoid*)0);

   glDrawArrays(GL_POINTS, 0, _width * _height);

   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
