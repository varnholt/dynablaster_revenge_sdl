#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "engine/render/watersurface.h"
#include "levels/level.h"

class GltfModel;
class GlowSprites;
class Matrix;

//! the floor and border of a story mode stage, tiled from the current world's glTF kit;
//! also scrolls the camera over stages larger than one screen
class StoryField
{
public:
   StoryField();
   ~StoryField();

   //! 1..8, every world has its own look and weather (weather= in data/game/story/world<n>/world.ini)
   void setWorld(int32_t world);

   [[nodiscard]] Level::Weather getWeather() const;

   //! field size in tiles, a new size jumps straight to the player
   void setSize(int32_t width, int32_t height);

   //! the next follow() jumps straight to the player instead of scrolling there
   void snap();

   //! moves the visible window towards the player (tile units), returns the offset in tiles
   void follow(float player_x, float player_y, float dt);

   [[nodiscard]] float getOffsetX() const;
   [[nodiscard]] float getOffsetY() const;

   //! who stands on the field (world xy), they leave foam where it's flooded
   void setWaders(std::vector<WaterSurface::Wader> waders);

   void render();

   //! tiles visible at once, like the original's single screen
   static constexpr int32_t SCREEN_WIDTH = 13;
   static constexpr int32_t SCREEN_HEIGHT = 11;

private:
   struct Kit
   {
      std::unique_ptr<GltfModel> _floor;
      std::unique_ptr<GltfModel> _border;

      //! the border's variants (nodes border_0, border_1, ...), empty if it has a single block
      std::vector<int32_t> _border_variants;
      std::unique_ptr<GltfModel> _corner;
      std::unique_ptr<GltfModel> _surroundings;

      //! scenery per field size (surroundings_<width>x<height>.glb), in field coordinates
      std::map<std::pair<int32_t, int32_t>, std::unique_ptr<GltfModel>> _scenery;
   };

   Kit& getKit();

   //! the world's scenery for the current field size, invalid if the world has none for it
   GltfModel& getScenery(Kit& kit);

   //! the border pieces around the field, transform comes after their own placement
   void drawBorder(Kit& kit, const Matrix& transform);

   std::map<int32_t, Kit> _kits;
   int32_t _world = 1;
   Level::Weather _weather = Level::Weather::Clear;
   int32_t _width = SCREEN_WIDTH;
   int32_t _height = SCREEN_HEIGHT;
   float _offset_x = 0.0f;
   float _offset_y = 0.0f;
   bool _snap = true;

   //! the lights of the scenery last drawn
   std::unique_ptr<GlowSprites> _glows;
   const GltfModel* _glow_source = nullptr;

   //! reflection and refraction of the scenery's water
   std::unique_ptr<WaterSurface> _water;
   std::vector<WaterSurface::Wader> _waders;
};
