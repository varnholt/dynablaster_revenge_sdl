#include "infectedflowfieldanimation.h"

#include "framework/gldevice.h"
#include "framework/framebuffer.h"

namespace
{
constexpr int PARTICLE_GRID_WIDTH = 64;
constexpr int PARTICLE_GRID_HEIGHT = 48;
constexpr float PARTICLE_SIZE = 60.0f;
constexpr float DISSOLVE_TIME = 300.0f;

GLuint g_quad_vertex_buffer = 0;

void drawQuad(const float* verts, int floats_per_vertex)
{
   const int order[6] = {0, 1, 2, 0, 2, 3};
   float buffer[6 * 4];

   for (int i = 0; i < 6; i++)
   {
      const float* src = verts + order[i] * floats_per_vertex;
      float* dst = buffer + i * floats_per_vertex;
      for (int c = 0; c < floats_per_vertex; c++)
      {
         dst[c] = src[c];
      }
   }

   if (g_quad_vertex_buffer == 0)
   {
      glGenBuffers(1, &g_quad_vertex_buffer);
   }

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
   {
      glDisableVertexAttribArray(1);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
}  // namespace

InfectedFlowFieldAnimation::InfectedFlowFieldAnimation()
   : _initialized(false),
     _width(0),
     _height(0),
     _vertex_pos_buffer(0),
     _vertex_color_buffer(0),
     _vertex_param_texture(0),
     _param_target(0),
     _vertex_color_texture(0),
     _color_target(0),
     _page(0),
     _center(0.0f, 0.0f, 0.0f),
     _flow_scale(0.2f),
     _particle_size(0.0f),
     _elapsed(0.0f),
     _stop(false)
{
   _positions[0] = _positions[1] = 0;
   _pos_target[0] = _pos_target[1] = 0;
}

InfectedFlowFieldAnimation::~InfectedFlowFieldAnimation()
{
   glDeleteFramebuffers(1, &_param_target);
   glDeleteFramebuffers(1, &_color_target);
   glDeleteFramebuffers(1, &_pos_target[0]);
   glDeleteFramebuffers(1, &_pos_target[1]);
   glDeleteTextures(1, &_vertex_param_texture);
   glDeleteTextures(1, &_vertex_color_texture);
   glDeleteTextures(1, &_positions[0]);
   glDeleteTextures(1, &_positions[1]);
   glDeleteBuffers(1, &_vertex_pos_buffer);
   glDeleteBuffers(1, &_vertex_color_buffer);
}

bool InfectedFlowFieldAnimation::initialized() const
{
   return _initialized;
}

void InfectedFlowFieldAnimation::setInitialized(bool init)
{
   _initialized = init;
}

const Vector& InfectedFlowFieldAnimation::getCenter() const
{
   return _center;
}

void InfectedFlowFieldAnimation::setCenter(const Vector& center)
{
   _center = center;
}

float InfectedFlowFieldAnimation::getScale() const
{
   return _flow_scale;
}

float InfectedFlowFieldAnimation::getPointSize() const
{
   return _particle_size;
}

void InfectedFlowFieldAnimation::stop()
{
   _stop = true;
}

bool InfectedFlowFieldAnimation::stopped() const
{
   return _stop;
}

bool InfectedFlowFieldAnimation::isElapsed() const
{
   return _elapsed > DISSOLVE_TIME;
}

void InfectedFlowFieldAnimation::initialize()
{
   _width = PARTICLE_GRID_WIDTH;
   _height = PARTICLE_GRID_HEIGHT;
   _particle_size = PARTICLE_SIZE;

   glGenTextures(1, &_vertex_param_texture);
   glBindTexture(GL_TEXTURE_2D, _vertex_param_texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &_param_target);
   glBindFramebuffer(GL_FRAMEBUFFER, _param_target);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _vertex_param_texture, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   glGenTextures(1, &_vertex_color_texture);
   glBindTexture(GL_TEXTURE_2D, _vertex_color_texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, _width, _height, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &_color_target);
   glBindFramebuffer(GL_FRAMEBUFFER, _color_target);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _vertex_color_texture, 0);
   // transparent until a particle respawns and picks up a color (original's clear color)
   static_cast<GLDevice*>(activeDevice)->clear(0.0f, 0.0f, 0.0f, 0.0f);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

   for (int i = 0; i < 2; i++)
   {
      glGenFramebuffers(1, &_pos_target[i]);
      glBindFramebuffer(GL_FRAMEBUFFER, _pos_target[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _positions[i], 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
   }

   glGenBuffers(1, &_vertex_pos_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_pos_buffer);
   glBufferData(GL_ARRAY_BUFFER, _width * _height * sizeof(Vector4), 0, GL_DYNAMIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glGenBuffers(1, &_vertex_color_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_color_buffer);
   glBufferData(GL_ARRAY_BUFFER, _width * _height * sizeof(Vector4), 0, GL_DYNAMIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InfectedFlowFieldAnimation::initializePositions(unsigned int depth_map, const Vector& min, const Vector& max)
{
   glBindFramebuffer(GL_FRAMEBUFFER, _pos_target[1]);
   glViewport(0, 0, _width, _height);
   glBindTexture(GL_TEXTURE_2D, depth_map);

   const float quad[4 * 4] = {
      -1.0f, -1.0f, min.x, min.y, 1.0f, -1.0f, max.x, min.y, 1.0f, 1.0f, max.x, max.y, -1.0f, 1.0f, min.x, max.y,
   };
   drawQuad(quad, 4);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::initializeParams(unsigned int depth_map, const Vector& min, const Vector& max)
{
   glBindFramebuffer(GL_FRAMEBUFFER, _param_target);
   glViewport(0, 0, _width, _height);
   glBindTexture(GL_TEXTURE_2D, depth_map);

   const float quad[4 * 4] = {
      -1.0f, -1.0f, min.x, min.y, 1.0f, -1.0f, max.x, min.y, 1.0f, 1.0f, max.x, max.y, -1.0f, 1.0f, min.x, max.y,
   };
   drawQuad(quad, 4);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::updatePositions(float delta)
{
   if (_stop)
   {
      _elapsed += delta;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, _pos_target[_page]);
   glViewport(0, 0, _width, _height);
   _page ^= 1;

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _positions[_page]);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, _vertex_param_texture);

   glActiveTexture(GL_TEXTURE0);

   const float quad[4 * 2] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
   drawQuad(quad, 2);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::updateColors()
{
   glBindFramebuffer(GL_FRAMEBUFFER, _color_target);
   glViewport(0, 0, _width, _height);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _positions[1 - _page]);
   glActiveTexture(GL_TEXTURE0);

   // a particle only gets a fresh color the frame it respawns; the shader discards every other
   // texel (the original's alpha test) so existing colors are kept.
   const float quad[4 * 2] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
   drawQuad(quad, 2);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::draw()
{
   FrameBuffer* prev = FrameBuffer::Instance();

   glBindFramebuffer(GL_FRAMEBUFFER, _color_target);
   glViewport(0, 0, _width, _height);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, _vertex_color_buffer);
   glReadPixels(0, 0, _width, _height, GL_RGBA, GL_FLOAT, 0);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

   glBindFramebuffer(GL_FRAMEBUFFER, _pos_target[1 - _page]);
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
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(activeDevice->getBorderLeft(), activeDevice->getBorderBottom(), activeDevice->getWidth(), activeDevice->getHeight());
   }

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_pos_buffer);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (GLvoid*)0);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_color_buffer);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (GLvoid*)0);

   glDrawArrays(GL_POINTS, 0, _width * _height);

   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
