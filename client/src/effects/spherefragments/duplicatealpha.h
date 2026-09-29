#pragma once

#include "postproduction/fullscreenquad.h"

#include <cstdint>

class Vector4;

class DuplicateAlpha
{
public:
   DuplicateAlpha();

   void process(uint32_t texture, const Vector4& color);

private:
   uint32_t _shader = 0;
   int32_t _color_param = -1;

   FullScreenQuad _quad;
};
