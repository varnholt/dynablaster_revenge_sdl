#pragma once

#include <cstdint>
#include <vector>
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

   DestructionMaterial();
   DestructionMaterial(
      const std::string& color_map,
      const std::string& environment_map,
      const std::string& specular_map,
      const std::string& shadow_map,
      Camera& shadow_camera
   );

   void update(float frame, const Matrix& camera) override;
   void load(Stream& stream) override;
   void addGeometry(Geometry& geometry) override;
   void renderDiffuse() override;
   void setLightProbes(const std::string& diffuse_map, const std::string& specular_map) override;
   void setShadowCookie(const std::string& shadow_map) override;

   //! swaps the debris' colour image (the stones' unwrapped cube), uploaded on the next render
   void setColorMap(const std::string& color_map);

   //! the debris of glass blocks: shards in their block's tint (see BlockMaterial::setGlass), lit from inside;
   //! empty: the stones' own colour
   void setGlass(std::vector<Vector> tints);


private:
   // a breaking glass block's flash and its light fading out
   void drawBreakingLight();

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
   int32_t _param_glass = 0;
   int32_t _param_tint = 0;
   std::vector<Vector> _glass_tints;

   std::optional<std::reference_wrapper<Camera>> _shadow_camera;
};
