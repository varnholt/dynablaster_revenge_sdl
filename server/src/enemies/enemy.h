#pragma once

#include <cstdint>
#include <numbers>
#include <vector>

// shared
#include "constants.h"
#include "enemytype.h"

class EnemyWorld;

//! a story mode monster walking the field's grid; subclasses decide where to go
class Enemy
{
public:
   Enemy(int16_t id, EnemyType type, EnemyWorld& world);
   virtual ~Enemy();

   //! called once the enemy is placed
   virtual void initialize();

   //! timers, behaviour and movement for one server tick
   void update(float dt);

   //! a flame reached the enemy, returns true if that killed it
   bool hit(int8_t player_id);

   //! removed without a fight (e.g. replaced by pontans when the time is up)
   void remove();

   [[nodiscard]] int16_t getId() const;
   [[nodiscard]] EnemyType getType() const;
   [[nodiscard]] int16_t getParentId() const;
   void setParentId(int16_t id);

   [[nodiscard]] float getX() const;
   [[nodiscard]] float getY() const;
   [[nodiscard]] int32_t getTileX() const;
   [[nodiscard]] int32_t getTileY() const;
   void setPosition(float x, float y);

   //! facing in radians, 0 = right, pi/2 = up (towards smaller y)
   [[nodiscard]] float getAngle() const;

   [[nodiscard]] Constants::Direction getDirection() const;
   void setDirection(Constants::Direction direction);

   //! tiles per second
   [[nodiscard]] float getSpeed() const;
   void setSpeed(float speed);

   [[nodiscard]] bool hasWallPass() const;
   void setWallPass(bool wall_pass);

   [[nodiscard]] int32_t getHitPoints() const;
   void setHitPoints(int32_t hit_points);

   [[nodiscard]] int32_t getPoints() const;
   void setPoints(int32_t points);

   //! shielded enemies shrug off any flame
   [[nodiscard]] bool isShielded() const;
   void setShielded(bool shielded);

   //! briefly true after a survived hit or protect()
   [[nodiscard]] bool isInvulnerable() const;

   //! nothing can hurt the enemy for the given seconds
   void protect(float seconds);

   [[nodiscard]] bool isDead() const;
   [[nodiscard]] bool isRemoved() const;
   [[nodiscard]] int8_t getKillerId() const;
   [[nodiscard]] bool isMoving() const;

   //! true once if anything visible changed since the last call
   [[nodiscard]] bool takeChanged();

   //! true once if the enemy survived a hit since the last call
   [[nodiscard]] bool takeHit();

   //! fires timeout(id) after ms milliseconds
   void startTimer(int32_t ms, int32_t id);

   //! directions that can be walked from the current tile
   [[nodiscard]] std::vector<Constants::Direction> getFreeDirections() const;
   [[nodiscard]] bool canMove(Constants::Direction direction) const;

   [[nodiscard]] EnemyWorld& getWorld() const;

protected:
   //! at a tile's centre: where to go next, DirectionUnknown stops; blocked is a mask of 1 << direction
   virtual Constants::Direction think(int32_t blocked);

   //! behaviour that isn't bound to tile centres
   virtual void tick(float dt);

   virtual void timeout(int32_t id);

   //! survived a hit
   virtual void hurt();

   //! the enemy is dying
   virtual void died();

   EnemyWorld& _world;

private:
   struct PendingTimer
   {
      float _remaining = 0.0f;
      int32_t _id = 0;
   };

   void move(float dt);
   void chooseDirection();
   [[nodiscard]] bool isAtCentre() const;

   int16_t _id = 0;
   EnemyType _type = EnemyType::Ballom;
   int16_t _parent_id = -1;
   float _x = 0.5f;
   float _y = 0.5f;
   float _angle = 1.5f * std::numbers::pi_v<float>;
   Constants::Direction _direction = Constants::DirectionUnknown;
   float _speed = 1.0f;
   bool _wall_pass = false;
   int32_t _hit_points = 1;
   int32_t _points = 100;
   bool _shielded = false;
   float _invulnerable_time = 0.0f;
   float _rethink_time = 0.0f;
   bool _dead = false;
   bool _removed = false;
   int8_t _killer_id = -1;
   bool _changed = true;
   bool _was_hit = false;
   bool _moved = false;
   std::vector<PendingTimer> _timers;
};
