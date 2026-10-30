#pragma once

#include <array>
#include <cstdint>
#include <vector>
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

   BlockMaterial();
   BlockMaterial(
      const std::string& color_map,
      const std::string& diffuse_map,
      const std::string& specular_map,
      const std::string& shadow_map,
      Camera& shadow_camera,
      bool ambient = true
   );

   void update(float frame, const Matrix& camera) override;
   void load(Stream& stream) override;
   void addGeometry(Geometry& geometry) override;
   void renderDiffuse() override;
   void setLightProbes(const std::string& diffuse_map, const std::string& specular_map) override;
   void setShadowCookie(const std::string& shadow_map) override;

   //! swaps the blocks' colour image (the unwrapped cube, e.g. "stone-unwrap"), uploaded on the next render
   void setColorMap(const std::string& color_map);

   //! every block shows its colour turned its own way (the top a quarter turn or more, the sides each other's)
   void setVaried(bool varied);

   //! draws the blocks as glass lit from inside (the story's crystal blocks): what's behind them shows through,
   //! their far edges inside, every block in one of tints (rgb 0..1, chosen by where it stands); empty: opaque
   void setGlass(std::vector<Vector> tints);

   //! whether the blocks are glass lit from inside (setGlass()): they light the floor round them, cast no shadow
   [[nodiscard]] bool isGlass() const;

   //! copies the frame drawn so far: what shows through glass blocks; call it once the ground is drawn and before
   //! the scene (anything drawn between would show at the blocks' own places)
   static void captureBehind();

   //! which of the glass tints a block standing at transform takes (the block and its light on the floor agree)
   static int32_t glassTint(const Matrix& transform);

   //! a glass block's light on the floor round its foot, added to what's there: strength how bright, reach how far
   //! out (in block widths from its middle); between beginLightPools() and endLightPools()
   static void beginLightPools();
   static void drawLightPool(const Matrix& transform, const Vector& tint, float strength, float reach);
   static void endLightPools();

private:
   void putImage(Image& target, int32_t x_position, int32_t y_position, const Image& source);
   void putScanline(std::span<uint32_t> destination, std::span<const uint32_t> source);

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
   int32_t _param_variation = 0;
   bool _varied = false;

   // glass: the blocks' tints and their extent in their own space
   std::vector<Vector> _glass_tints;
   Vector _box_min{1e9f, 1e9f, 1e9f};
   Vector _box_max{-1e9f, -1e9f, -1e9f};
   int32_t _param_glass = 0;
   std::array<int32_t, 3> _param_tints{};
   int32_t _param_viewport = 0;
   int32_t _param_box_min = 0;
   int32_t _param_box_max = 0;
   int32_t _param_refraction = 0;
   int32_t _param_tint_choice = 0;
   int32_t _param_caustics = 0;
   int32_t _param_caustic_color = 0;
   int32_t _param_water_level = 0;
   int32_t _param_clock = 0;

   // the glass blocks' light on the floor round their feet, drawn after them
   void drawLightPools();

   std::optional<std::reference_wrapper<Camera>> _shadow_camera;
   Matrix _camera;
};
