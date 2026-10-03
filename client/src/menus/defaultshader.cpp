#include "defaultshader.h"
#include "framework/gldevice.h"

namespace
{
uint32_t _shader = 0;
int _alpha_param = -1;

uint32_t _blit_shader = 0;
int _blit_alpha_param = -1;
}  // namespace

uint32_t getDefaultMenuShader()
{
   if (_shader == 0)
   {
      _shader = activeDevice().loadShader("data/shaders/texalpha-vert.glsl", "data/shaders/texalpha-frag.glsl");
      _alpha_param = activeDevice().getParameterIndex("alpha");
   }

   return _shader;
}

int getDefaultMenuShaderAlphaParam()
{
   getDefaultMenuShader();
   return _alpha_param;
}

uint32_t getFramebufferBlitShader()
{
   if (_blit_shader == 0)
   {
      _blit_shader = activeDevice().loadShader("data/shaders/texalphaignore-vert.glsl", "data/shaders/texalphaignore-frag.glsl");
      _blit_alpha_param = activeDevice().getParameterIndex("alpha");
   }

   return _blit_shader;
}

int getFramebufferBlitShaderAlphaParam()
{
   getFramebufferBlitShader();
   return _blit_alpha_param;
}
