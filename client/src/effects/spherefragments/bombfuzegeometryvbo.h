#pragma once

#include "geometryvbo.h"

#include "render/texture.h"

#include <cstdint>

class Geometry;
class Vector4;

class BombFuzeGeometryVbo : public GeometryVbo
{
public:
   explicit BombFuzeGeometryVbo(Geometry* geometry);

   void initialize() override;

   void draw(const Vector4& color);

   void initGlParameters();

   void cleanupGlParameter();

protected:
   Texture _texture;

   uint32_t _shader = 0;
   int32_t _fresnel = -1;
   int32_t _color_param = -1;
};
