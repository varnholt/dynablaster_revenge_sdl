#include "extraanimations.h"

#include "framework/globaltime.h"
#include "gldevice.h"
#include "image/image.h"
#include "math/matrix.h"
#include "math/vector.h"
#include "math/vector4.h"
#include "menus/defaultshader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <string>

namespace
{
// reveal timings are in game ticks (62.5 per second)
constexpr float REVEAL_GROW_TIME = 50.0f;
constexpr float REVEAL_OPEN_TIME = 50.0f;
constexpr float REVEAL_ANIMATION_TIME = 75.0f;
constexpr float REVEAL_FADE_OUT_START_TIME = 50.0f;
constexpr float REVEAL_FADE_OUT_DURATION = 25.0f;
constexpr float FRUSTUM_TOP_SCALE = 1.5f;
constexpr float FRUSTUM_HEIGHT = 3.0f;
constexpr float FRUSTUM_BOTTOM_SCALE = 0.75f;

// destroyed timings are in milliseconds
constexpr float DESTROYED_TIME_EXPAND = 500.0f;
constexpr float DESTROYED_RING_SCALE = 3.5f;

// texture flags for GLDevice::createTexture
constexpr int32_t TEXTURE_LINEAR = 1;
constexpr int32_t TEXTURE_CLAMP = 4;

constexpr int32_t FLOATS_PER_VERTEX = 5;

float nowMs()
{
   return GlobalTime::Instance().getTime() * 1000.0f;
}

uint32_t loadTexture(const std::string& name, int32_t flags)
{
   Image image;
   image.load(name);
   return activeDevice().createTexture(image.getData(), image.getWidth(), image.getHeight(), flags);
}

// appends one quad as two triangles, corners given counter-clockwise as (x, y, z, u, v)
void appendQuad(std::vector<float>& out, const std::array<std::array<float, FLOATS_PER_VERTEX>, 4>& corners)
{
   for (const auto index : {0, 1, 2, 0, 2, 3})
   {
      out.insert(out.end(), corners[index].begin(), corners[index].end());
   }
}
}  // namespace

ExtraAnimations::ExtraAnimations()
{
   _frustum_texture = loadTexture("data/game/extra_frustum", TEXTURE_LINEAR | TEXTURE_CLAMP);
   _ring_texture = loadTexture("data/game/extrasphere_full_red", TEXTURE_LINEAR);

   _reveal_shader = activeDevice().loadShader("extrareveal-vert.glsl", "extrareveal-frag.glsl");
   _reveal_color_param = activeDevice().getParameterIndex("color");
   _reveal_scroll_param = activeDevice().getParameterIndex("textureScroll");
   _reveal_texture_param = activeDevice().getParameterIndex("texturemap");

   glGenBuffers(1, &_vertex_buffer);
}

ExtraAnimations::~ExtraAnimations()
{
   activeDevice().deleteTexture(_frustum_texture);
   activeDevice().deleteTexture(_ring_texture);
   glDeleteBuffers(1, &_vertex_buffer);
}

void ExtraAnimations::addReveal(int32_t x, int32_t y)
{
   _reveals.push_back({x, y, 0.0f});
}

void ExtraAnimations::addDestroyed(int32_t x, int32_t y)
{
   _destroyed.push_back({static_cast<float>(x) + 0.5f, -static_cast<float>(y) - 0.5f, nowMs()});
}

void ExtraAnimations::clear()
{
   _reveals.clear();
   _destroyed.clear();
}

void ExtraAnimations::render(float dt)
{
   const float now = nowMs();
   std::erase_if(_destroyed, [now](const Destroyed& ring) { return now - ring.start_time > DESTROYED_TIME_EXPAND; });
   std::erase_if(_reveals, [](const Reveal& reveal) { return reveal.elapsed > REVEAL_ANIMATION_TIME; });

   if (_reveals.empty() && _destroyed.empty())
   {
      return;
   }

   renderDestroyed();
   renderReveals(dt);

   activeDevice().setShader(0);

   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ExtraAnimations::renderDestroyed()
{
   if (_destroyed.empty())
   {
      return;
   }

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);

   activeDevice().setShader(getDefaultMenuShader());

   // world space vertices, the camera lives in the projection matrix - push() uploads the
   // transform to the bound shader, so it has to follow setShader()
   activeDevice().push(Matrix());
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _ring_texture);

   const float now = nowMs();

   for (const auto& ring : _destroyed)
   {
      const float normalized_time = std::min((now - ring.start_time) / DESTROYED_TIME_EXPAND, 1.0f);
      const float quad_size = normalized_time * DESTROYED_RING_SCALE;
      const float scale = 0.5f * quad_size;
      const float z = quad_size * 0.4f;

      activeDevice().setParameter(getDefaultMenuShaderAlphaParam(), 1.0f - normalized_time);

      std::vector<float> vertices;
      appendQuad(
         vertices,
         {{
            {ring.x - scale, ring.y - scale, z, 0.0f, 0.0f},
            {ring.x + scale, ring.y - scale, z, 1.0f, 0.0f},
            {ring.x + scale, ring.y + scale, z, 1.0f, 1.0f},
            {ring.x - scale, ring.y + scale, z, 0.0f, 1.0f},
         }}
      );
      drawTriangles(vertices);
   }

   activeDevice().pop();
}

void ExtraAnimations::renderReveals(float dt)
{
   if (_reveals.empty())
   {
      return;
   }

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE);
   glDepthMask(GL_FALSE);

   // all four frustum sides are visible from outside and inside
   activeDevice().setCulling(false);

   activeDevice().setShader(_reveal_shader);
   activeDevice().push(Matrix());
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _frustum_texture);
   activeDevice().bindSampler(_reveal_texture_param, 0);

   for (auto& reveal : _reveals)
   {
      reveal.elapsed += dt;

      const float bottom_offset = (1.0f - FRUSTUM_BOTTOM_SCALE) * 0.5f;
      const float top_scale = std::min(reveal.elapsed / REVEAL_OPEN_TIME, 1.0f) * FRUSTUM_TOP_SCALE;
      const float top_offset = (1.0f - top_scale) * 0.5f;

      const float b0 = bottom_offset;
      const float b1 = bottom_offset + FRUSTUM_BOTTOM_SCALE;
      const float t0 = top_offset;
      const float t1 = top_offset + top_scale;
      const float h = std::min(reveal.elapsed / REVEAL_GROW_TIME, 1.0f) * FRUSTUM_HEIGHT;

      float alpha = 1.0f;
      if (reveal.elapsed > REVEAL_FADE_OUT_START_TIME)
      {
         alpha = std::min(1.0f, (reveal.elapsed - REVEAL_FADE_OUT_START_TIME) / REVEAL_FADE_OUT_DURATION);
         alpha = 0.5f * (1.0f + std::cos(alpha * std::numbers::pi_v<float>));
      }

      activeDevice().setParameter(_reveal_color_param, Vector4(1.0f, 1.0f, 1.0f, alpha));
      activeDevice().setParameter(_reveal_scroll_param, Vector(reveal.elapsed, -reveal.elapsed, 0.0f) * 0.005f);

      // the four sides of the frustum, in the tile's local space (y and z are flipped)
      const float ox = static_cast<float>(reveal.x);
      const float oy = -static_cast<float>(reveal.y) - 1.0f;

      std::vector<float> vertices;
      const auto side = [&](float u0, float u1, float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3)
      {
         appendQuad(
            vertices,
            {{
               {ox + x0, oy + y0, 0.0f, u0, 1.0f},
               {ox + x1, oy + y1, 0.0f, u1, 1.0f},
               {ox + x2, oy + y2, h, u1, 0.0f},
               {ox + x3, oy + y3, h, u0, 0.0f},
            }}
         );
      };

      side(0.5f, 0.75f, b0, b1, b1, b1, t1, t1, t0, t1);  // back
      side(0.0f, 0.25f, b0, b0, b1, b0, t1, t0, t0, t0);  // front
      side(0.75f, 1.0f, b0, b0, b0, b1, t0, t1, t0, t0);  // left
      side(0.25f, 0.5f, b1, b0, b1, b1, t1, t1, t1, t0);  // right

      drawTriangles(vertices);
   }

   activeDevice().pop();
   activeDevice().setCulling(true);
}

void ExtraAnimations::drawTriangles(std::span<const float> vertices)
{
   const auto vertex_count = static_cast<int32_t>(vertices.size() / FLOATS_PER_VERTEX);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float) * FLOATS_PER_VERTEX * vertex_count, vertices.data(), GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_VERTEX, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_VERTEX, reinterpret_cast<const GLvoid*>(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, vertex_count);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
