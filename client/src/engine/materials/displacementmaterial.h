#pragma once

#include <cstdint>
#include "../render/renderbuffer.h"
#include "../render/uv.h"
#include "material.h"
#include "math/matrix.h"

class DisplacementMaterial : public Material
{
public:
   struct Vertex
   {
      Vector position;
      UV uv;
   };

   DisplacementMaterial(SceneGraph* scene);
   DisplacementMaterial(SceneGraph* scene, const char* map, const char* diffuse_map);
   void load(Stream* stream) override;
   void addGeometry(Geometry* geometry) override;
   void renderDiffuse() override;
   void update(float frame, Node** node_list, const Matrix& camera) override;

private:
   void begin() override;
   void end() override;
   void init() override;

   Texture _color_map;
   Texture _diffuse_map;
   uint32_t _shader = 0;

   int32_t _param_texture = 0;
   int32_t _param_diffuse = 0;
   int32_t _param_time = 0;

   Matrix _camera;
};
