#pragma once

#include "filter.h"
#include "math/vector2.h"

#include <cstdint>

class MotionBlurFilter : public Filter
{
public:
   MotionBlurFilter();

   bool init() override;
   void process(uint32_t texture, float u, float v) override;

   void setIntensity(float intensity);
   float getIntensity() const;

   void setMotionDir(const Vector2& direction);
   const Vector2& getMotionDir() const;

private:
   uint32_t _shader = 0;

   int32_t _intensity_param = -1;
   float _intensity = 0.0f;

   int32_t _motion_dir_param = -1;
   Vector2 _motion_dir;
};
