#include "motionblurfilter.h"

#include "framework/gldevice.h"

MotionBlurFilter::MotionBlurFilter() : Filter("motionblur")
{
}

bool MotionBlurFilter::init()
{
   _shader = activeDevice().loadShader("motionblur-vert.glsl", "motionblur-frag.glsl");

   _intensity_param = activeDevice().getParameterIndex("intensity");
   _motion_dir_param = activeDevice().getParameterIndex("motionDir");

   return true;
}

void MotionBlurFilter::process(uint32_t, float, float)
{
   activeDevice().setShader(_shader);
   activeDevice().setParameter(_intensity_param, _intensity);
   activeDevice().setParameter(_motion_dir_param, _motion_dir);
}

void MotionBlurFilter::setIntensity(float intensity)
{
   _intensity = intensity;
}

float MotionBlurFilter::getIntensity() const
{
   return _intensity;
}

void MotionBlurFilter::setMotionDir(const Vector2& direction)
{
   _motion_dir = direction;
}

const Vector2& MotionBlurFilter::getMotionDir() const
{
   return _motion_dir;
}
