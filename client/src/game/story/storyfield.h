#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>

#include "levels/level.h"

class GltfModel;

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

   void render();

   //! tiles visible at once, like the original's single screen
   static constexpr int32_t SCREEN_WIDTH = 13;
   static constexpr int32_t SCREEN_HEIGHT = 11;

private:
   struct Kit
   {
      std::unique_ptr<GltfModel> _floor;
      std::unique_ptr<GltfModel> _border;
      std::unique_ptr<GltfModel> _corner;
      std::unique_ptr<GltfModel> _surroundings;
   };

   Kit& getKit();

   std::map<int32_t, Kit> _kits;
   int32_t _world = 1;
   Level::Weather _weather = Level::Weather::Clear;
   int32_t _width = SCREEN_WIDTH;
   int32_t _height = SCREEN_HEIGHT;
   float _offset_x = 0.0f;
   float _offset_y = 0.0f;
   bool _snap = true;
};
