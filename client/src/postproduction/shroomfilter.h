#pragma once

#include "filter.h"
#include "fullscreenquad.h"

class ShroomFilter : public Filter
{
public:
   ShroomFilter();
   ~ShroomFilter() override;

   bool init() override;
   void process(unsigned int texture, float u, float v) override;

   //! snapshots the current viewport and draws it back distorted
   void apply();

   void setIntensity(float intensity);
   void setTime(float time);

private:
   unsigned int _shader = 0;
   int _intensity_param = -1;
   int _time_param = -1;
   int _texture_param = -1;

   float _intensity = 0.0f;
   float _time = 0.0f;

   unsigned int _snapshot_texture = 0;
   FullScreenQuad _quad;
};
