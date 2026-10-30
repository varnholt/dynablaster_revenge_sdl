#pragma once

#include "../level.h"

#include "math/vector.h"

#include <memory>
#include <random>
#include <vector>

class GlowSprites;
class GltfModel;
class SeaSpray;
class WaterSurface;

//! the deck of a pirate ship on the Caribbean sea from morning to night: the ship is a glTF scenery with its light
//! baked in by day and by night (assets/level-pirate), its level.hjb only holds the cameras; the sea swells round
//! her, the ship and the field stay put, and now and then a wave slaps against her side
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
   //! a wave slapping against the hull every few seconds: its spray and the foam it leaves on the sea
   void splash(float time, float level);

   std::unique_ptr<GltfModel> _ship;
   std::unique_ptr<WaterSurface> _water;

   //! the lanterns on board, the moon in the sky (it moves with the swell)
   std::unique_ptr<GlowSprites> _glows;
   std::unique_ptr<GlowSprites> _sky_glows;

   std::unique_ptr<SeaSpray> _spray;

   //! when and where the waves broke, their foam fades on the water
   struct Splash
   {
      float _time = 0.0f;
      Vector _at;
   };

   std::vector<Splash> _splashes;
   float _next_splash = -1.0f;
   float _last_time = -1.0f;
   std::mt19937 _random{1717};

   //! which of the light probes from day (0) to night the blocks and the players are lit by
   int32_t _probe_step = 0;

   //! 1 by day, 0 by night
   float _day = 1.0f;

   //! 0..1, how much of the evening's or the morning's warm light there is
   float _dusk = 0.0f;
};
