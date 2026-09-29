#pragma once

#include <cstdint>
#include "../render/renderbuffer.h"
#include "../render/uv.h"
#include "material.h"
#include "math/matrix.h"

class ExtraMapping : public Material
{
public:
   struct Vertex
   {
      Vector position;
      UV uv;
   };

   ExtraMapping(SceneGraph* scene);
   ExtraMapping(SceneGraph* scene, const char* map);
   void load(Stream* stream) override;
   void addGeometry(Geometry* geometry) override;
   void update(float frame, Node** node_list, const Matrix& camera) override;
   void renderDiffuse() override;

private:
   void init() override;
   void begin() override;
   void end() override;

   Texture _color_map;
   uint32_t _shader = 0;

   int32_t _param_texture = 0;

   Matrix _camera;
};
