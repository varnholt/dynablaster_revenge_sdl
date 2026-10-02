#include "shroomfilter.h"

#include "framework/framebuffer.h"
#include "framework/gldevice.h"

#include <cstdint>

ShroomFilter::ShroomFilter() : Filter("shroom")
{
}

ShroomFilter::~ShroomFilter()
{
   if (_snapshot_texture)
   {
      glDeleteTextures(1, &_snapshot_texture);
   }
}

bool ShroomFilter::init()
{
   _shader = activeDevice->loadShader("mushroom-vert.glsl", "mushroom-frag.glsl");

   _intensity_param = activeDevice->getParameterIndex("intensity");
   _time_param = activeDevice->getParameterIndex("time");
   _texture_param = activeDevice->getParameterIndex("texture0");

   glGenTextures(1, &_snapshot_texture);

   return true;
}

void ShroomFilter::apply()
{
   int32_t x = 0;
   int32_t y = 0;
   int32_t width = 0;
   int32_t height = 0;
   activeDevice->getViewPort(&x, &y, &width, &height);

   // sampling the framebuffer we draw into would be a feedback loop, so copy it first
   glBindTexture(GL_TEXTURE_2D, _snapshot_texture);
   FrameBuffer::copyTexImage(x, y, width, height);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   process(_snapshot_texture, 1.0f, 1.0f);
}

void ShroomFilter::process(uint32_t texture, float u, float v)
{
   activeDevice->setShader(_shader);

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, texture);
   activeDevice->bindSampler(_texture_param, 0);

   activeDevice->setParameter(_intensity_param, _intensity);
   activeDevice->setParameter(_time_param, _time);

   // slight zoom so the distorted edges stay off screen
   const float scale = 1.0f + 0.1f * _intensity;
   _quad.drawRect(-scale, -scale, scale, scale, 0.0f, 0.0f, u, v);

   activeDevice->setShader(0);

   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
}

void ShroomFilter::setIntensity(float intensity)
{
   _intensity = intensity;
}

void ShroomFilter::setTime(float time)
{
   _time = time;
}
