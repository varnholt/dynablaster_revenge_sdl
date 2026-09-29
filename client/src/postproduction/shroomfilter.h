#pragma once

#include "filter.h"
#include "fullscreenquad.h"
#include "gles3.h"

#include <cstdint>

class ShroomFilter : public Filter
{
public:
   ShroomFilter();
   ~ShroomFilter() override;

   bool init() override;
   void process(uint32_t texture, float u, float v) override;

   // snapshots the current viewport and draws it back distorted
   void apply();

   void setIntensity(float intensity);
   void setTime(float time);

private:
   uint32_t _shader = 0;
   int32_t _intensity_param = -1;
   int32_t _time_param = -1;
   int32_t _texture_param = -1;

   float _intensity = 0.0f;
   float _time = 0.0f;

   GLuint _snapshot_texture = 0;
   FullScreenQuad _quad;
};
