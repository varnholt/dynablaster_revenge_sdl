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
   unsigned int mShader;

   int mIntensityParam;
   float mIntensity;

   int mMotionDirParam;
   Vector2 mMotionDir;
};
