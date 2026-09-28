#include "motionblurfilter.h"

#include "framework/gldevice.h"

MotionBlurFilter::MotionBlurFilter()
   : Filter("motionblur"),
     _shader(0),
     _intensity_param(-1),
     _intensity(0.0f),
     _motion_dir_param(-1)
{
}

MotionBlurFilter::~MotionBlurFilter() = default;

bool MotionBlurFilter::init()
{
   _shader = activeDevice->loadShader("motionblur-vert.glsl", "motionblur-frag.glsl");

   _intensity_param = activeDevice->getParameterIndex("intensity");
   _motion_dir_param = activeDevice->getParameterIndex("motionDir");

   return true;
}

void MotionBlurFilter::process(unsigned int, float, float)
{
   activeDevice->setShader(_shader);
   activeDevice->setParameter(_intensity_param, _intensity);
   activeDevice->setParameter(_motion_dir_param, _motion_dir);
}

void MotionBlurFilter::setIntensity(float intensity)
{
   _intensity = intensity;
}

float MotionBlurFilter::getIntensity() const
{
   return _intensity;
}

void MotionBlurFilter::setMotionDir(const Vector2& dir)
{
   _motion_dir = dir;
}

const Vector2& MotionBlurFilter::getMotionDir() const
{
   return _motion_dir;
}
