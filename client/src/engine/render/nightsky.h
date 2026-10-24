#pragma once

#include <cstdint>
#include <string>

#include "math/vector.h"
#include "render/texture.h"

//! a night sky behind everything, seen from wherever the current camera looks: the soft sky from an
//! equirectangular map (z up), sharp twinkling stars and the moon drawn over it (nightsky-frag.glsl)
class NightSky
{
public:
   //! sky is the map's name for the TexturePool, without its extension
   explicit NightSky(const std::string& sky);
   ~NightSky();

   NightSky(const NightSky&) = delete;
   NightSky& operator=(const NightSky&) = delete;

   //! draws it with the current camera, first thing in a frame: it leaves the depth buffer as it is
   void render();

   //! 0 black .. 1
   void setBrightness(float brightness);

   //! towards the moon, z up, and its radius in radians
   void setMoon(const Vector& direction, float size);

private:
   Texture _sky;
   uint32_t _shader = 0;
   int32_t _inverse_param = -1;
   int32_t _sky_param = -1;
   int32_t _time_param = -1;
   int32_t _brightness_param = -1;
   int32_t _moon_param = -1;
   int32_t _moon_size_param = -1;
   uint32_t _vertex_buffer = 0;
   float _brightness = 1.0f;
   Vector _moon = Vector(-0.197f, 0.925f, 0.326f);
   float _moon_size = 0.035f;
};
