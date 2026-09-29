#pragma once

#include <cstdint>
#include "material.h"
#include "math/matrix.h"
#include "render/uv.h"

class EnvironmentMaterial : public Material
{
public:
   struct Vertex
   {
      Vector position;
      Vector normal;
   };

   EnvironmentMaterial(SceneGraph* scene);
   EnvironmentMaterial(SceneGraph* scene, const char* specular_map);

   void init() override;
   void update(float frame, Node** node_list, const Matrix& camera) override;
   void load(Stream* stream) override;
   void addGeometry(Geometry* geometry) override;
   void begin() override;
   void end() override;
   void renderDiffuse() override;

private:
   Texture _specular_map;
   uint32_t _shader = 0;

   int32_t _param_specular = 0;
   int32_t _param_camera = 0;

   Matrix _camera;
};
