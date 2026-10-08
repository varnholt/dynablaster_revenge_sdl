#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

// server
#include "enemies/enemyworld.h"

// shared
#include "storystage.h"
#include "storystatepacket.h"

class Enemy;
class Game;
class Map;
class Player;

//! the single player campaign through the original game's 64 stages
class StoryMode : public EnemyWorld
{
public:
   StoryMode(Game& game, int32_t first_stage);
   ~StoryMode() override;

   [[nodiscard]] const StoryStage& getStage() const;
   [[nodiscard]] int32_t getStageIndex() const;

   //! the field of the current stage, freshly generated for every attempt
   [[nodiscard]] std::unique_ptr<Map> createMap();

   //! abilities carried over from earlier stages and lives
   void applyPlayerState(Player& player) const;

   //! the map and players are placed, now the enemies
   void prepareStage();

   //! the countdown is over
   void startStage();

   //! one server tick while the stage is running
   void update(float dt);

   //! a bomb's flame reached a tile
   void flameReached(int32_t x, int32_t y, int8_t owner_id);

   //! the exit door was hit by a flame; it survives but releases enemies
   void exitBombed(int32_t x, int32_t y);

   //! the stage's item was burnt, releasing enemies
   void itemBombed(int32_t x, int32_t y);

   //! a player picked up an extra
   void extraCollected(Player& player, int32_t extra_type);

   //! every player is dead
   void playersDied();

   //! the stage time ran out: the remaining enemies are replaced by pontans
   void timeUp();

   //! game over or all stages done
   [[nodiscard]] bool isFinished() const;

   //! milliseconds until the next attempt or stage begins
   [[nodiscard]] int32_t getNextStageDelay() const;

   //! owner id of bombs and fire coming from enemies
   static constexpr int8_t ENEMY_OWNER_ID = -2;

   // EnemyWorld
   [[nodiscard]] int32_t getWidth() const override;
   [[nodiscard]] int32_t getHeight() const override;
   [[nodiscard]] bool isWalkable(int32_t x, int32_t y, bool wall_pass) const override;
   [[nodiscard]] bool isDangerous(int32_t x, int32_t y) const override;
   [[nodiscard]] std::optional<Point> getNearestPlayer(float x, float y) const override;
   [[nodiscard]] Constants::Direction getDirectionToPlayer(const Enemy& enemy, int32_t max_steps) const override;
   [[nodiscard]] std::optional<Point> getRandomFreeTile(int32_t min_player_distance) const override;
   int16_t spawnEnemy(EnemyType type, float x, float y, int16_t parent_id) override;
   [[nodiscard]] int32_t countEnemies(EnemyType type) const override;
   void spitFire(int32_t x, int32_t y, int32_t range) override;
   void dropBomb(int32_t x, int32_t y, int32_t flames) override;

private:
   struct CarriedAbilities
   {
      int32_t _bombs = 1;
      int32_t _flames = STORY_DEFAULT_FLAMECOUNT;
      float _speed = SERVER_DEFAULT_SPEED;
      bool _remote = false;
      bool _wall_pass = false;
      bool _bomb_pass = false;
   };

   void removeAllEnemies();
   void spawnStageEnemies();
   void spawnAround(int32_t x, int32_t y, int32_t count);
   void updateEnemies(float dt);
   void updateFlames(float dt);
   void updateTouches();
   void updateStageCleared();
   void stageCleared();
   void enemyDied(Enemy& enemy);
   void sendState(StoryStatePacket::State state);
   void killPlayer(Player& player);
   [[nodiscard]] int32_t aliveEnemyCount() const;
   [[nodiscard]] Map& getMap() const;
   [[nodiscard]] bool isPlayerOn(int32_t x, int32_t y) const;

   Game& _game;
   int32_t _stage_index = 0;
   int32_t _lives = STORY_LIVES;
   int32_t _score = 0;
   bool _game_over = false;
   bool _completed = false;
   bool _cleared = false;
   bool _time_up = false;
   float _boss_clear_delay = -1.0f;

   CarriedAbilities _abilities;

   std::optional<Point> _exit;
   std::vector<std::unique_ptr<Enemy>> _enemies;
   std::vector<std::unique_ptr<Enemy>> _spawned;
   int16_t _next_enemy_id = 0;

   //! lingering flames per tile (ms) and whether they hurt enemies (player bombs) or only players
   std::vector<float> _flame_time;
   std::vector<int8_t> _flame_owner;
};
