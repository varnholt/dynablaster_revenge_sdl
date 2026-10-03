#pragma once

#include <cstdint>
#include "material.h"
#include "math/matrix.h"
#include "render/uv.h"

class EnvironmentTextureMaterial : public Material
{
public:
   struct Vertex
   {
      Vector position;
      Vector normal;
      UV uv;
   };

   EnvironmentTextureMaterial();
   EnvironmentTextureMaterial(const std::string& color_map, const std::string& environment_map, const std::string& specular_map);

   void updateGeometry(const VertexBuffer& vertex_buffer, const Geometry& geometry);
   void update(float frame, const Matrix& camera) override;
   void load(Stream& stream) override;
   void addGeometry(Geometry& geometry) override;
   void renderDiffuse() override;

private:
   void init() override;
   void begin() override;
   void end() override;

   Texture _color_map;
   Texture _diffuse_map;
   Texture _specular_map;
   uint32_t _shader = 0;

   int32_t _param_specular = 0;
   int32_t _param_diffuse = 0;
   int32_t _param_texture = 0;
   int32_t _param_camera = 0;

   Matrix _camera;
};
