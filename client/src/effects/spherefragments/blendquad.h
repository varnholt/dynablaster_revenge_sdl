#pragma once

#include "math/vector.h"
#include "math/vector4.h"
#include "postproduction/fullscreenquad.h"

#include <cstdint>

class BlendQuad
{
public:
   BlendQuad();

   void process(uint32_t texture, const Vector4& color = Vector4(1, 1, 1, 1), float scale = 1.0f, const Vector& offset = Vector(0, 0, 0));

private:
   uint32_t _shader = 0;
   int32_t _color_param = -1;
   int32_t _scale_param = -1;
   int32_t _offset_param = -1;

   FullScreenQuad _quad;
};
