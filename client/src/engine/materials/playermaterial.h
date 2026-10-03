#pragma once

#include <cstdint>
#include "math/matrix.h"
#include "playermaterialbase.h"
#include "render/uv.h"

class PlayerMaterial : public PlayerMaterialBase
{
public:
   PlayerMaterial();
   PlayerMaterial(
      const std::string& color_map,
      const std::string& environment_map,
      const std::string& specular_map,
      const std::string& ambient_occlusion_map
   );

   void exportOBJ(Stream& stream, int32_t& index_offset) override;

   void load(Stream& stream) override;
   void renderDiffuse() override;

   void setColorMap(const Texture& color_map);

private:
   void init() override;
   void begin() override;
   void end() override;

   Texture _color_map;
   Texture _diffuse_map;
   Texture _specular_map;
   Texture _ambient_map;
   uint32_t _shader = 0;

   int32_t _param_specular = 0;
   int32_t _param_diffuse = 0;
   int32_t _param_texture = 0;
   int32_t _param_ambient = 0;
   int32_t _param_camera = 0;
   int32_t _param_bones = 0;
   int32_t _param_flash = 0;
   int32_t _param_fade = 0;
};
