#include "seaspray.h"

#include "gldevice.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace
{
// center, corner, size, opacity
constexpr int32_t FLOATS_PER_VERTEX = 7;

constexpr std::array<std::array<float, 2>, 6> CORNERS{{{-1, -1}, {1, -1}, {1, 1}, {-1, -1}, {1, 1}, {-1, 1}}};

constexpr float GRAVITY = 9.8f;

// a burst's droplets and puffs of mist at size 1
constexpr int32_t DROPLETS = 160;
constexpr int32_t MISTS = 26;

// no more than this many at once, a burst over it is cut short
constexpr size_t MAX_DROPLETS = 2000;
}  // namespace

SeaSpray::SeaSpray()
{
   _shader = activeDevice().loadShader("seaspray-vert.glsl", "seaspray-frag.glsl");
   _aspect_param = activeDevice().getParameterIndex("u_aspect");
   _color_param = activeDevice().getParameterIndex("u_color");

   glGenBuffers(1, &_vertex_buffer);
}

SeaSpray::~SeaSpray()
{
   glDeleteBuffers(1, &_vertex_buffer);
}

void SeaSpray::burst(const Vector& at, const Vector& outward, float size, const Vector& wind)
{
   std::uniform_real_distribution<float> unit(0.0f, 1.0f);
   Vector away(outward.x, outward.y, 0.0f);
   away.normalize();
   const Vector along(-away.y, away.x, 0.0f);

   // the wave breaks along a stretch of the hull, the water shoots up the side and is flung off it
   const int32_t droplets = static_cast<int32_t>(static_cast<float>(DROPLETS) * size);
   for (int32_t i = 0; i < droplets + MISTS && _droplets.size() < MAX_DROPLETS; i++)
   {
      const bool mist = i >= droplets;
      Droplet droplet;
      droplet._mist = mist;
      droplet._position = at + along * ((unit(_random) - 0.5f) * 1.6f * size) + away * (unit(_random) * 0.3f);

      // most of it a sheet straight up the hull, some flung far out; the strongest up past the rail
      const float rise = std::pow(unit(_random), 0.7f) * (mist ? 5.0f : 9.5f) * std::sqrt(size);
      const float spread = (0.3f + 1.8f * unit(_random) * unit(_random)) * size;
      droplet._velocity = away * spread + along * ((unit(_random) - 0.5f) * 1.2f) + Vector(0.0f, 0.0f, rise) + wind * 0.3f;
      droplet._size = mist ? (0.5f + 0.6f * unit(_random)) * size : (0.06f + 0.1f * unit(_random)) * std::sqrt(size);
      droplet._life = mist ? 1.6f + 1.4f * unit(_random) : 3.0f;

      // the mist rises from the spray's middle, a moment later
      droplet._age = mist ? -0.25f * unit(_random) : 0.0f;
      _droplets.push_back(droplet);
   }
}

void SeaSpray::animate(float dt, float level)
{
   for (Droplet& droplet : _droplets)
   {
      droplet._age += dt;
      if (droplet._age < 0.0f)
      {
         continue;
      }

      if (droplet._mist)
      {
         // slowed by the air, drifting off, swelling as it thins out
         droplet._velocity = droplet._velocity * std::exp(-1.6f * dt) + Vector(0.0f, 0.0f, -0.6f * dt);
         droplet._size *= 1.0f + 0.35f * dt;
      }
      else
      {
         droplet._velocity.z -= GRAVITY * dt;
         droplet._velocity = droplet._velocity * std::exp(-0.25f * dt);
      }

      droplet._position = droplet._position + droplet._velocity * dt;
   }

   std::erase_if(
      _droplets,
      [level](const Droplet& droplet)
      { return droplet._age > droplet._life || (!droplet._mist && droplet._position.z < level && droplet._velocity.z < 0.0f); }
   );
}

void SeaSpray::render(const Matrix& transform, const Vector& color)
{
   _vertices.clear();
   for (const Droplet& droplet : _droplets)
   {
      if (droplet._age < 0.0f)
      {
         continue;
      }

      // the mist comes and goes softly, the droplets thin out towards the end of their flight
      const float t = droplet._age / droplet._life;
      const float opacity =
         droplet._mist ? 0.5f * std::sin(std::numbers::pi_v<float> * std::min(t * 1.4f, 1.0f)) * (1.0f - t) : 0.85f * (1.0f - t * t);
      for (const auto& [u, v] : CORNERS)
      {
         _vertices.insert(
            _vertices.end(),
            {droplet._position.x, droplet._position.y, droplet._position.z, u, v, droplet._size, droplet._mist ? -opacity : opacity}
         );
      }
   }

   if (_vertices.empty())
   {
      return;
   }

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(_vertices.size() * sizeof(float)), _vertices.data(), GL_DYNAMIC_DRAW);

   const RenderDevice::ViewPort viewport = activeDevice().getViewPort();
   const float aspect = viewport.height > 0 ? static_cast<float>(viewport.width) / static_cast<float>(viewport.height) : 16.0f / 9.0f;

   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   activeDevice().setCulling(false);

   activeDevice().setShader(_shader);
   activeDevice().setParameter(_aspect_param, aspect);
   activeDevice().setParameter(_color_param, color);
   activeDevice().push(transform);

   for (GLuint attribute = 0; attribute < 3; attribute++)
   {
      glEnableVertexAttribArray(attribute);
   }

   constexpr GLsizei stride = sizeof(float) * FLOATS_PER_VERTEX;
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const GLvoid*>(sizeof(float) * 3));
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const GLvoid*>(sizeof(float) * 5));

   glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(_vertices.size() / FLOATS_PER_VERTEX));

   for (GLuint attribute = 0; attribute < 3; attribute++)
   {
      glDisableVertexAttribArray(attribute);
   }
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   activeDevice().pop();
   activeDevice().setShader(0);
   activeDevice().setCulling(true);
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);
}
