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

private:
   std::unique_ptr<GltfModel> _ship;
   std::unique_ptr<WaterSurface> _water;
   std::unique_ptr<GlowSprites> _glows;
};
