#include "duplicatealpha.h"
#include "gldevice.h"
#include "math/vector4.h"

DuplicateAlpha::DuplicateAlpha()
{
   _shader = activeDevice->loadShader("duplicatealpha-vert.glsl", "duplicatealpha-frag.glsl");

   _color_param = activeDevice->getParameterIndex("color");
}

void DuplicateAlpha::process(uint32_t texture, const Vector4& color)
{
   activeDevice->setShader(_shader);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   activeDevice->setParameter(_color_param, color);
   glBindTexture(GL_TEXTURE_2D, texture);

   _quad.drawUnit();

   activeDevice->setShader(0);
   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
}
