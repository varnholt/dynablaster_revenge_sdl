#include "glowsprites.h"

#include "framework/globaltime.h"
#include "gldevice.h"

#include <array>
#include <cmath>

namespace
{
// center, corner, color, size + mode + phase
constexpr int32_t FLOATS_PER_VERTEX = 11;

// the mode of a fire's flame quad, after GltfModel's modes
constexpr float FLAME_BODY = 5.0f;

constexpr std::array<std::array<float, 2>, 6> CORNERS{{{-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1}}};
}  // namespace

GlowSprites::GlowSprites()
{
   _shader = activeDevice().loadShader("glow-vert.glsl", "glow-frag.glsl");
   _time_param = activeDevice().getParameterIndex("u_time");
   _brightness_param = activeDevice().getParameterIndex("u_brightness");
   _aspect_param = activeDevice().getParameterIndex("u_aspect");

   glGenBuffers(1, &_vertex_buffer);
}

GlowSprites::~GlowSprites()
{
   glDeleteBuffers(1, &_vertex_buffer);
}

void GlowSprites::set(const std::vector<GltfModel::Glow>& glows)
{
   std::vector<float> vertices;
   vertices.reserve(glows.size() * CORNERS.size() * FLOATS_PER_VERTEX);

   for (const GltfModel::Glow& glow : glows)
   {
      // lights next to each other don't blink in step
      const float phase = std::fmod(std::abs(glow._position.x * 12.9898f + glow._position.y * 78.233f), 100.0f);

      // a fire is its halo and, drawn by the same shader, the flame's own quad
      const bool fire = glow._mode == GltfModel::GlowFire;
      for (int32_t part = 0; part < (fire ? 2 : 1); part++)
      {
         const float mode = part == 0 ? static_cast<float>(glow._mode) : FLAME_BODY;

         for (const auto& [u, v] : CORNERS)
         {
            vertices.insert(
               vertices.end(),
               {glow._position.x,
                glow._position.y,
                glow._position.z,
                u,
                v,
                glow._color.x,
                glow._color.y,
                glow._color.z,
                glow._size,
                mode,
                phase}
            );
         }
      }
   }

   _vertex_count = static_cast<int32_t>(vertices.size() / FLOATS_PER_VERTEX);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GlowSprites::render(const Matrix& transform)
{
   if (_vertex_count == 0)
   {
      return;
   }

   const RenderDevice::ViewPort viewport = activeDevice().getViewPort();
   const float aspect = viewport.height > 0 ? static_cast<float>(viewport.width) / static_cast<float>(viewport.height) : 16.0f / 9.0f;

   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   // premultiplied: the halos add (alpha 0), a fire's flame also covers a little of what's behind it
   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
   activeDevice().setCulling(false);

   activeDevice().setShader(_shader);
   activeDevice().setParameter(_time_param, GlobalTime::Instance().getTime());
   activeDevice().setParameter(_aspect_param, aspect);
   activeDevice().setParameter(_brightness_param, _brightness);
   activeDevice().push(transform);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   for (GLuint attribute = 0; attribute < 4; attribute++)
   {
      glEnableVertexAttribArray(attribute);
   }

   constexpr GLsizei stride = sizeof(float) * FLOATS_PER_VERTEX;
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const GLvoid*>(sizeof(float) * 3));
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const GLvoid*>(sizeof(float) * 5));
   glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const GLvoid*>(sizeof(float) * 8));

   glDrawArrays(GL_TRIANGLES, 0, _vertex_count);

   for (GLuint attribute = 0; attribute < 4; attribute++)
   {
      glDisableVertexAttribArray(attribute);
   }
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   activeDevice().pop();
   activeDevice().setShader(0);
   activeDevice().setCulling(true);
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GlowSprites::setBrightness(float brightness)
{
   _brightness = brightness;
}
