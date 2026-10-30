#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "math/matrix.h"
#include "math/vector.h"

//! spray thrown up where a wave slaps against a hull: droplets flying up and falling back into the sea, a veil of
//! mist drifting off with the wind; camera facing, soft, blended over what's behind them
class SeaSpray
{
public:
   SeaSpray();
   ~SeaSpray();

   SeaSpray(const SeaSpray&) = delete;
   SeaSpray& operator=(const SeaSpray&) = delete;

   //! a splash at the sea's surface at (world space), thrown out away from the hull along outward (xy) and up,
   //! size about 1 for a wave against the ship's side; wind (xy, units a second) carries the mist and the spray
   void burst(const Vector& at, const Vector& outward, float size, const Vector& wind);

   //! moves the droplets on by dt seconds, those back in the sea (below level) are gone
   void animate(float dt, float level);

   //! draws them with the current camera after the opaque geometry and the water, transform moves them (the
   //! sea's swell); colour the white of the spray in the light there is
   void render(const Matrix& transform, const Vector& color);

private:
   struct Droplet
   {
      Vector _position;
      Vector _velocity;
      float _size = 0.1f;
      float _age = 0.0f;
      float _life = 1.0f;

      //! mist drifts and swells, droplets fall
      bool _mist = false;
   };

   std::vector<Droplet> _droplets;
   std::vector<float> _vertices;
   std::mt19937 _random{1492};

   uint32_t _shader = 0;
   int32_t _aspect_param = -1;
   int32_t _color_param = -1;
   uint32_t _vertex_buffer = 0;
};
