#include "brightnessfilter.h"

#include "framework/gldevice.h"

BrightnessFilter::BrightnessFilter(float gamma) : Filter("brightness"), _gamma(gamma)
{
}

bool BrightnessFilter::init()
{
   _shader = activeDevice().loadShader("brightness-vert.glsl", "brightness-frag.glsl");

   _gamma_param = activeDevice().getParameterIndex("gamma");
   _texture_param = activeDevice().getParameterIndex("texture0");

   return true;
}

void BrightnessFilter::setGamma(float gamma)
{
   _gamma = gamma;
}

void BrightnessFilter::process(uint32_t texture, float u, float v)
{
   activeDevice().setShader(_shader);

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   activeDevice().bindSampler(_texture_param, 0);

   activeDevice().setParameter(_gamma_param, _gamma);

   _quad.drawRect(-1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, u, v);

   activeDevice().setShader(0);

   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
}
