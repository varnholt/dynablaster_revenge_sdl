#include "motionblurfilter.h"

#include "framework/gldevice.h"

MotionBlurFilter::MotionBlurFilter()
   : Filter("motionblur"),
     mShader(0),
     mIntensityParam(-1),
     mIntensity(0.0f),
     mMotionDirParam(-1)
{
}

MotionBlurFilter::~MotionBlurFilter() = default;

bool MotionBlurFilter::init()
{
   mShader = activeDevice->loadShader("motionblur-vert.glsl", "motionblur-frag.glsl");

   mIntensityParam = activeDevice->getParameterIndex("intensity");
   mMotionDirParam = activeDevice->getParameterIndex("motionDir");

   return true;
}

void MotionBlurFilter::process(unsigned int, float, float)
{
   activeDevice->setShader(mShader);
   activeDevice->setParameter(mIntensityParam, mIntensity);
   activeDevice->setParameter(mMotionDirParam, mMotionDir);
}

void MotionBlurFilter::setIntensity(float intensity)
{
   mIntensity = intensity;
}

float MotionBlurFilter::getIntensity() const
{
   return mIntensity;
}

void MotionBlurFilter::setMotionDir(const Vector2& dir)
{
   mMotionDir = dir;
}

const Vector2& MotionBlurFilter::getMotionDir() const
{
   return mMotionDir;
}
