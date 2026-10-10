#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

// shared
#include "constants.h"
#include "enemytype.h"
#include "point.h"

class Enemy;

//! what an enemy can see of and do in the game around it
class EnemyWorld
{
public:
   virtual ~EnemyWorld() = default;

   [[nodiscard]] virtual int32_t getWidth() const = 0;
   [[nodiscard]] virtual int32_t getHeight() const = 0;

   //! false for pillars, bombs and the field's border; soft blocks only for wall passing enemies
   [[nodiscard]] virtual bool isWalkable(int32_t x, int32_t y, bool wall_pass) const = 0;

   //! a bomb is about to go off with its flames reaching this tile
   [[nodiscard]] virtual bool isDangerous(int32_t x, int32_t y) const = 0;

   //! position of the nearest living player in tiles
   [[nodiscard]] virtual std::optional<Point> getNearestPlayer(float x, float y) const = 0;

   //! first step on the shortest walk towards the nearest player, DirectionUnknown if there's none
   [[nodiscard]] virtual Constants::Direction getDirectionToPlayer(const Enemy& enemy, int32_t max_steps) const = 0;

   //! a free tile far away from the players, for teleporting enemies
   [[nodiscard]] virtual std::optional<Point> getRandomFreeTile(int32_t min_player_distance) const = 0;

   //! another enemy appears; returns its id
   virtual int16_t spawnEnemy(EnemyType type, float x, float y, int16_t parent_id) = 0;

   //! number of living enemies of the given type
   [[nodiscard]] virtual int32_t countEnemies(EnemyType type) const = 0;

   //! enemy fire travelling up to range tiles into each direction, only harming players
   virtual void spitFire(int32_t x, int32_t y, int32_t range) = 0;

   //! enemies dropping bombs (black bomberman)
   virtual void dropBomb(int32_t x, int32_t y, int32_t flames) = 0;
};
