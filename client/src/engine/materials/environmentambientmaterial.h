#pragma once

#include <cstdint>
#include "material.h"
#include "math/matrix.h"
#include "render/uv.h"

class EnvironmentAmbientMaterial : public Material
{
public:
   struct Vertex
   {
      Vector position;
      Vector normal;
      UV uv;
   };

   EnvironmentAmbientMaterial(SceneGraph* scene);
   EnvironmentAmbientMaterial(SceneGraph* scene, const char* ambient_map, const char* specular_map);

   void update(float frame, Node** node_list, const Matrix& camera) override;
   void load(Stream* stream) override;
   void addGeometry(Geometry* geometry) override;
   void renderDiffuse() override;

private:
   void init() override;
   void begin() override;
   void end() override;

   Texture _specular_map;
   Texture _ambient_map;
   uint32_t _shader = 0;

   int32_t _param_specular = 0;
   int32_t _param_ambient = 0;
   int32_t _param_camera = 0;

   Matrix _camera;
};
