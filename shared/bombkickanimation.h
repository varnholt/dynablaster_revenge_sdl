#pragma once

#include "constants.h"
#include "point.h"
#include "signal.h"
#include "timer.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

class BombMapItem;
class Map;
class MapItem;

class BombKickAnimation
{
public:
   BombKickAnimation();
   virtual ~BombKickAnimation();

   // remove all animations
   static void deleteAll();

   // runs right before this animation is destroyed - used by subscribers of longer-lived signals
   // to disconnect themselves, since Signal<> has no automatic disconnect-on-destroy
   void addDestroyCallback(std::function<void()> callback);

   void start();

   void setMap(Map* map);
   [[nodiscard]] Map* getMap() const;

   void setX(float x);
   void setY(float y);
   [[nodiscard]] float getX() const;
   [[nodiscard]] float getY() const;

   [[nodiscard]] bool isReadyToExplode() const;
   void setReadyToExplode(bool ready);

   void setDirection(Constants::Direction direction);

   // ignite animation at x, y
   static void ignite(int32_t x, int32_t y);

   // bomb may explode now
   Signal<> explodeSignal;

   Signal<Constants::Direction, float> startedSignal;
   Signal<> stoppedSignal;

   // enable the ready-to-explode flag, or explode if it was already set
   void readyToExplode();

   void updatePlayerPosition(int32_t id, float x, float y);

   // remove player position if player died
   void removePlayerPosition(int32_t id);

protected:
   void updatePosition();

   bool isInRange(float value1, float value2, float epsilon);

   // movement may be continued
   bool isMoveAllowed();

   [[nodiscard]] float getStepSize() const;
   [[nodiscard]] Constants::Direction getDirection() const;
   [[nodiscard]] int32_t getDirectionX() const;
   [[nodiscard]] int32_t getDirectionY() const;

   void unmapBomb();
   void remapBomb();

   void reset();

   // inter-bomb collisions
   static void addAnimation(BombKickAnimation* animation);
   static void removeAnimation(BombKickAnimation* animation);
   bool checkCollision(BombKickAnimation* animation);
   [[nodiscard]] bool isColliding() const;
   void setColliding(bool colliding);
   void updateCollisions();

   // animation update timer
   Timer _timer;

   // intensity factor
   float _factor;

   Constants::Direction _direction = Constants::DirectionUnknown;
   float _x = 0.0f;
   float _y = 0.0f;
   bool _ready_to_explode = false;
   Map* _map = nullptr;
   BombMapItem* _bomb_map_item = nullptr;

   // player positions to collide with
   std::unordered_map<int32_t, Point> _player_positions;

   // run in the destructor, see addDestroyCallback()
   std::vector<std::function<void()>> _destroy_callbacks;

   // currently active kick animations - non-owning, self-registering tracker (added in the
   // constructor, removed in the destructor); the real owner is the BombMapItem holding it.
   // TODO: deleteAll() deletes through these non-owning pointers, which double-deletes any
   // animation whose owning BombMapItem (or a pending deferred-delete) outlives the call.
   static std::vector<BombKickAnimation*> _animations;

   // bomb is colliding with another bomb
   bool _colliding = false;
};
