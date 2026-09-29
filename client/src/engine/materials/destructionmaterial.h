#pragma once

#include <cstdint>
#include "material.h"
#include "math/matrix.h"
#include "render/uv.h"

class Camera;

class DestructionMaterial : public Material
{
public:
   struct Vertex
   {
      Vector position;
      Vector normal;
      UV uv;
   };

   DestructionMaterial(SceneGraph* scene);
   DestructionMaterial(
      SceneGraph* scene,
      const char* color_map,
      const char* environment_map,
      const char* specular_map,
      const char* shadow_map,
      Camera* shadow_camera
   );

   void update(float frame, Node** node_list, const Matrix& camera) override;
   void load(Stream* stream) override;
   void addGeometry(Geometry* geometry) override;
   void renderDiffuse() override;

private:
   void init() override;
   void begin() override;
   void end() override;

   Texture _color_map;
   Texture _diffuse_map;
   Texture _specular_map;
   Texture _shadow_map;
   uint32_t _shader = 0;

   int32_t _param_specular = 0;
   int32_t _param_diffuse = 0;
   int32_t _param_texture = 0;
   int32_t _param_shadow = 0;
   int32_t _param_shadow_camera = 0;

   Camera* _shadow_camera = nullptr;
};
