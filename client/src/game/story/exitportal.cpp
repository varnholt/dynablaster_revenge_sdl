#include "exitportal.h"

#include "framework/globaltime.h"
#include "gldevice.h"
#include "math/matrix.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
// game ticks (62.5 per second) the portal takes to open
constexpr float OPEN_TIME = 75.0f;

constexpr int32_t SEGMENTS = 48;

// tile units, the portal is centered on its tile
constexpr float HOLE_RADIUS = 0.34f;
constexpr float LID_RADIUS = 0.36f;
constexpr float LID_HEIGHT = 0.03f;
constexpr float FUNNEL_DEPTH = 0.9f;
constexpr float FUNNEL_BOTTOM_RADIUS = 0.05f;
constexpr float GLOW_RADIUS = 1.25f;
constexpr float PUNCH_HEIGHT = 0.05f;

// position, uv, normal
constexpr int32_t FLOATS_PER_VERTEX = 8;

struct ProfilePoint
{
   float _r = 0.0f;
   float _z = 0.0f;
   float _v = 0.0f;
};

void appendVertex(std::vector<float>& out, float angle, const ProfilePoint& point, float nr, float nz)
{
   const float c = std::cos(angle);
   const float s = std::sin(angle);
   out.insert(
      out.end(),
      {point._r * c, point._r * s, point._z, angle / (2.0f * std::numbers::pi_v<float>), point._v, nr * c, nr * s, nz}
   );
}

// spins a profile around the z axis, flat shaded per profile segment
void appendLathe(std::vector<float>& out, const std::vector<ProfilePoint>& profile)
{
   for (size_t i = 0; i + 1 < profile.size(); ++i)
   {
      const ProfilePoint& p0 = profile[i];
      const ProfilePoint& p1 = profile[i + 1];

      const float dr = p1._r - p0._r;
      const float dz = p1._z - p0._z;
      const float length = std::hypot(dr, dz);
      const float nr = -dz / length;
      const float nz = dr / length;

      for (int32_t segment = 0; segment < SEGMENTS; ++segment)
      {
         const float a0 = 2.0f * std::numbers::pi_v<float> * static_cast<float>(segment) / SEGMENTS;
         const float a1 = 2.0f * std::numbers::pi_v<float> * static_cast<float>(segment + 1) / SEGMENTS;

         appendVertex(out, a0, p0, nr, nz);
         appendVertex(out, a1, p0, nr, nz);
         appendVertex(out, a1, p1, nr, nz);
         appendVertex(out, a0, p0, nr, nz);
         appendVertex(out, a1, p1, nr, nz);
         appendVertex(out, a0, p1, nr, nz);
      }
   }
}

void appendDisc(std::vector<float>& out, float radius, float z)
{
   appendLathe(out, {{0.0f, z, 0.0f}, {radius, z, 1.0f}});
}
}  // namespace

ExitPortal::ExitPortal()
{
   _shader = activeDevice().loadShader("exitportal-vert.glsl", "exitportal-frag.glsl");
   _part_param = activeDevice().getParameterIndex("part");
   _time_param = activeDevice().getParameterIndex("time");
   _openness_param = activeDevice().getParameterIndex("openness");
   _aperture_param = activeDevice().getParameterIndex("aperture");

   glGenBuffers(1, &_vertex_buffer);
   buildGeometry();
}

ExitPortal::~ExitPortal()
{
   glDeleteBuffers(1, &_vertex_buffer);
}

void ExitPortal::buildGeometry()
{
   std::vector<float> vertices;

   const auto add_part = [&](auto&& append)
   {
      const auto first = static_cast<int32_t>(vertices.size() / FLOATS_PER_VERTEX);
      append();
      _ranges.push_back({first, static_cast<int32_t>(vertices.size() / FLOATS_PER_VERTEX) - first});
   };

   // Part::Rim, a rounded stone ring rising a little out of the floor
   add_part(
      [&]
      {
         appendLathe(
            vertices,
            {{0.33f, 0.0f, 0.0f},
             {0.35f, 0.05f, 0.25f},
             {0.39f, 0.08f, 0.45f},
             {0.43f, 0.08f, 0.55f},
             {0.47f, 0.05f, 0.75f},
             {0.49f, 0.0f, 1.0f}}
         );
      }
   );

   // Part::Funnel, curving down into the depth, v runs from the top to the bottom
   add_part(
      [&]
      {
         std::vector<ProfilePoint> profile;
         constexpr int32_t RINGS = 12;
         for (int32_t ring = RINGS; ring >= 0; --ring)
         {
            const float t = static_cast<float>(ring) / RINGS;
            const float r = FUNNEL_BOTTOM_RADIUS + (HOLE_RADIUS - FUNNEL_BOTTOM_RADIUS) * std::pow(t, 0.6f);
            profile.push_back({r, -FUNNEL_DEPTH * (1.0f - t), 1.0f - t});
         }
         appendLathe(vertices, profile);
         appendDisc(vertices, FUNNEL_BOTTOM_RADIUS, -FUNNEL_DEPTH);
      }
   );

   // Part::Lid
   add_part([&] { appendDisc(vertices, LID_RADIUS, LID_HEIGHT); });

   // Part::Glow, a square is enough, the shader fades it out radially
   add_part(
      [&]
      {
         for (const auto& [x, y] : {std::pair{-1.0f, -1.0f}, {1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, -1.0f}, {1.0f, 1.0f}, {-1.0f, 1.0f}})
         {
            vertices.insert(vertices.end(), {x * GLOW_RADIUS, y * GLOW_RADIUS, PUNCH_HEIGHT, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f});
         }
      }
   );

   // Part::DepthPunch, a unit disc scaled to the iris' opening
   add_part([&] { appendDisc(vertices, 1.0f, PUNCH_HEIGHT); });

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(float)), vertices.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ExitPortal::show(int32_t x, int32_t y)
{
   _shown = true;
   _x = x;
   _y = y;
   _open = false;
   _openness = 0.0f;
}

void ExitPortal::hide()
{
   _shown = false;
}

bool ExitPortal::isShown() const
{
   return _shown;
}

bool ExitPortal::isAt(int32_t x, int32_t y) const
{
   return _shown && _x == x && _y == y;
}

int32_t ExitPortal::getX() const
{
   return _x;
}

int32_t ExitPortal::getY() const
{
   return _y;
}

bool ExitPortal::open()
{
   if (!_shown || _open)
   {
      return false;
   }

   _open = true;
   return true;
}

void ExitPortal::draw(Part part)
{
   const Range& range = _ranges[static_cast<size_t>(part)];
   glUniform1i(_part_param, static_cast<GLint>(part));
   glDrawArrays(GL_TRIANGLES, range._first, range._count);
}

void ExitPortal::render(float dt)
{
   if (!_shown)
   {
      return;
   }

   if (_open)
   {
      _openness = std::min(_openness + dt / OPEN_TIME, 1.0f);
   }

   // the iris starts turning once the blue box has grown a little
   const float iris = std::clamp(_openness * 1.4f - 0.2f, 0.0f, 1.0f);
   const float aperture = LID_RADIUS * iris * iris * (3.0f - 2.0f * iris);

   const Matrix center = Matrix::position(static_cast<float>(_x) + 0.5f, -static_cast<float>(_y) - 0.5f, 0.0f);

   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);
   activeDevice().setCulling(false);

   activeDevice().setShader(_shader);
   activeDevice().setParameter(_time_param, GlobalTime::Instance().getTime());
   activeDevice().setParameter(_openness_param, _openness);
   activeDevice().setParameter(_aperture_param, aperture);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_VERTEX, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_VERTEX, reinterpret_cast<const GLvoid*>(sizeof(float) * 3));
   glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_VERTEX, reinterpret_cast<const GLvoid*>(sizeof(float) * 5));

   if (aperture > 0.0f)
   {
      // cut the floor open: clear its depth inside the opening, the lid covers the seam
      const float punch_radius = std::min(aperture * 1.1f, LID_RADIUS);
      activeDevice().push(Matrix::scale(punch_radius, punch_radius, 1.0f) * center);
      // depth only, the blend keeps the color buffer as it is
      glEnable(GL_BLEND);
      glBlendFunc(GL_ZERO, GL_ONE);
      glDepthFunc(GL_ALWAYS);
      draw(Part::DepthPunch);
      glDepthFunc(GL_LEQUAL);
      glDisable(GL_BLEND);

      activeDevice().push(center);
      draw(Part::Funnel);
   }

   activeDevice().push(center);
   draw(Part::Rim);

   if (aperture < LID_RADIUS)
   {
      draw(Part::Lid);
   }

   if (_openness > 0.0f)
   {
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE);
      glDepthMask(GL_FALSE);
      draw(Part::Glow);
   }

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   activeDevice().pop();
   activeDevice().setShader(0);
   activeDevice().setCulling(true);
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
