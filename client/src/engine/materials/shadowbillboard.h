#pragma once

#include <cstdint>
#include <unordered_map>
#include "../render/renderbuffer.h"
#include "../render/uv.h"
#include "material.h"
#include "math/vector2.h"

class ShadowBillboard : public Material
{
public:
   struct Vertex
   {
      Vector position;
      UV uv;
   };

   struct Bounding
   {
      Vector min;
      Vector max;
   };

   ShadowBillboard(SceneGraph* scene);
   ShadowBillboard(SceneGraph* scene, const char* map);
   void load(Stream& stream) override;
   void addGeometry(Geometry* geometry) override;
   void renderDiffuse() override;
   void removeMesh(Mesh* mesh) override;
   void setOffset(float x, float y);

private:
   static constexpr int32_t max_billboards = 1000;

   void init() override;
   void begin() override;
   void end() override;

   Texture _color_map;
   uint32_t _shader = 0;
   uint32_t _vertices = 0;
   uint32_t _texcoords = 0;
   uint32_t _indices = 0;

   int32_t _param_texture = 0;
   std::unordered_map<Geometry*, Bounding> _instances;
   Vector2 _offset{0.0f, 0.0f};
};
