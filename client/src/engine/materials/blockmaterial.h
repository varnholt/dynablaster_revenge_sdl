#pragma once

#include <cstdint>
#include "image/psd.h"
#include "material.h"
#include "math/matrix.h"
#include "render/uv.h"

class Camera;

class BlockMaterial : public Material
{
public:
   struct Vertex
   {
      Vector position;
      Vector normal;
      UV uv;
   };

   BlockMaterial(SceneGraph* scene);
   BlockMaterial(
      SceneGraph* scene,
      const char* color_map,
      const char* diffuse_map,
      const char* specular_map,
      const char* shadow_map,
      Camera* shadow_camera,
      bool ambient = true
   );

   void update(float frame, Node** node_list, const Matrix& camera) override;
   void load(Stream* stream) override;
   void addGeometry(Geometry* geometry) override;
   void renderDiffuse() override;

private:
   void putImage(Image& target, int32_t x_position, int32_t y_position, const Image& source);
   void putScanline(uint32_t* destination, const uint32_t* source, int32_t width);

   void init() override;
   void begin() override;
   void end() override;

   Texture _diffuse_map;
   Texture _color_map;
   Texture _ambient_map;
   Texture _shadow_map;
   Texture _specular_map;
   uint32_t _shader = 0;

   int32_t _param_diffuse = 0;
   int32_t _param_texture = 0;
   int32_t _param_ambient = 0;
   int32_t _param_shadow = 0;
   int32_t _param_specular = 0;
   int32_t _param_camera = 0;
   int32_t _param_shadow_camera = 0;
   int32_t _param_offset = 0;

   Camera* _shadow_camera = nullptr;
   Matrix _camera;
};
