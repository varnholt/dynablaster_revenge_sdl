#pragma once

#include <cstdint>
#include "math/matrix.h"
#include "playermaterialbase.h"
#include "render/uv.h"

class InvisibilityMaterial : public PlayerMaterialBase
{
public:
   InvisibilityMaterial();

   void load(Stream& stream) override;
   void renderDiffuse() override;

   void setTexture(uint32_t texture);

private:
   void init() override;
   void begin() override;
   void end() override;

   uint32_t _shader = 0;

   uint32_t _texture_map = 0;
   Texture _gradient_map;

   int32_t _param_texture = 0;
   int32_t _param_gradient = 0;
   int32_t _param_fade_threshold = 0;
   int32_t _param_camera = 0;
   int32_t _param_bones = 0;
};
