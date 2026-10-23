#pragma once

#include "../level.h"

#include <memory>

class GlowSprites;
class GltfModel;
class WaterSurface;

//! the deck of a pirate ship at night: the ship is a glTF scenery (tools/level-pirate), its level.hjb only
//! holds the cameras
class LevelPirate : public Level
{
public:
   LevelPirate();
   ~LevelPirate() override;

   void loadData() override;
   void drawLevel(float frame, const Matrix& view) override;

   //! a day over the game's rounds: morning, noon, sunset, night, dawn
   void setDayTime(float phase) override;

private:
   std::unique_ptr<GltfModel> _ship;
   std::unique_ptr<WaterSurface> _water;
   std::unique_ptr<GlowSprites> _glows;

   //! 1 by day, 0 by night
   float _day = 1.0f;

   //! 0..1, how much of the evening's or the morning's warm light there is
   float _dusk = 0.0f;
};
