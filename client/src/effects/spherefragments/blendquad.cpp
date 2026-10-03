#include "blendquad.h"
#include "gldevice.h"

BlendQuad::BlendQuad()
{
   _shader = activeDevice().loadShader("blendquad-vert.glsl", "blendquad-frag.glsl");

   _color_param = activeDevice().getParameterIndex("color");
   _scale_param = activeDevice().getParameterIndex("scale");
   _offset_param = activeDevice().getParameterIndex("offset");
}

void BlendQuad::process(uint32_t texture, const Vector4& color, float scale, const Vector& offset)
{
   activeDevice().setShader(_shader);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   activeDevice().setParameter(_color_param, color);
   activeDevice().setParameter(_scale_param, scale);
   activeDevice().setParameter(_offset_param, offset);
   glBindTexture(GL_TEXTURE_2D, texture);

   _quad.drawUnit();

   activeDevice().setShader(0);
   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
}
