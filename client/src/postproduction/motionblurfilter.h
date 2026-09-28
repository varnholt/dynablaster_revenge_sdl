#pragma once

#include "filter.h"
#include "math/vector2.h"

class MotionBlurFilter : public Filter
{
public:
   MotionBlurFilter();
   ~MotionBlurFilter();

   bool init();
   void process(unsigned int texture, float u, float v);

   void setIntensity(float intensity);
   float getIntensity() const;

   void setMotionDir(const Vector2& dir);
   const Vector2& getMotionDir() const;

private:
   unsigned int _shader;

   int _intensity_param;
   float _intensity;

   int _motion_dir_param;
   Vector2 _motion_dir;
};
