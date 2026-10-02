#pragma once

#include "filter.h"
#include "fullscreenquad.h"

#include <cstdint>

// draws a texture with the gamma of the brightness setting
class BrightnessFilter : public Filter
{
public:
   explicit BrightnessFilter(float gamma = 2.2f);

   bool init() override;
   void process(uint32_t texture, float u, float v) override;

   void setGamma(float gamma);

private:
   uint32_t _shader = 0;
   int32_t _gamma_param = -1;
   int32_t _texture_param = -1;

   float _gamma = 2.2f;

   FullScreenQuad _quad;
};
