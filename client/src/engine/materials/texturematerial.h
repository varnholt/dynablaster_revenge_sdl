#pragma once

#include <cstdint>
#include "../render/uv.h"
#include "material.h"

class TextureMaterial : public Material
{
public:
   struct Vertex
   {
      Vector position;
      Vector normal;
      UV uv;
   };

   TextureMaterial();
   TextureMaterial(const std::string& texture_map);

   void load(Stream& stream) override;
   void addGeometry(Geometry& geometry) override;
   void update(float frame, const Matrix& camera) override;

   void renderDiffuse() override;

private:
   void init() override;
   void begin() override;
   void end() override;

   Texture _color_map;
   uint32_t _shader = 0;

   int32_t _param_texture = 0;
};
