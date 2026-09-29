#pragma once

#include "math/matrix.h"
#include "math/vector.h"
#include "render/texture.h"

#include <array>
#include <cstdint>

// falling snow over the castle level, drawn as additive point sprites
class SnowAnimation
{
public:
   SnowAnimation();
   ~SnowAnimation();

   SnowAnimation(const SnowAnimation&) = delete;
   SnowAnimation& operator=(const SnowAnimation&) = delete;

   //! \param dt game ticks since the last frame (62.5 per second)
   void animate(float dt);

   //! \param transform snow space (unit square, z up) to world space
   void draw(const Matrix& transform);

   //! hides all flakes until they respawn after the round's countdown
   void reset();

private:
   static constexpr int32_t PARTICLE_COUNT = 2000;

   struct Particle
   {
      Vector position = Vector(0.0f, 0.0f, 0.0f);
      float size = 1.0f;
      float speed = 1.0f;
      bool visible = false;
   };

   void initialize();
   void initParticle(Particle& particle, float z);
   void updateWind(float elapsed_ms);
   float elapsedMs() const;

   std::array<Particle, PARTICLE_COUNT> _particles;
   Vector _wind_direction = Vector(0.0f, 0.0f, 0.0f);
   float _start_time = 0.0f;

   Texture _texture;
   uint32_t _vertex_buffer = 0;
   uint32_t _shader = 0;
   int32_t _texture_param = -1;
   int32_t _size_factor_param = -1;
};
