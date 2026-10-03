#pragma once

#include "constants.h"
#include "gamesignal.h"
#include "point.h"
#include "timer.h"

#include <cstdint>
#include <functional>
#include <map>
#include <unordered_map>
#include <vector>

class Map;

class BombKickAnimation
{
public:
   explicit BombKickAnimation(Map& map);
   virtual ~BombKickAnimation();

   BombKickAnimation(const BombKickAnimation&) = delete;
   BombKickAnimation& operator=(const BombKickAnimation&) = delete;

   // runs right before this animation is destroyed - used by subscribers of longer-lived signals
   // to disconnect themselves, since Signal<> has no automatic disconnect-on-destroy
   void addDestroyCallback(std::function<void()> callback);

   void start();

   // stop moving for good, e.g. once the bomb exploded
   void stop();

   [[nodiscard]] Map& getMap() const;

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

   bool isInRange(float value1, float value2, float epsilon) const;

   // movement may be continued
   bool isMoveAllowed() const;

   [[nodiscard]] float getStepSize() const;
   [[nodiscard]] Constants::Direction getDirection() const;
   [[nodiscard]] int32_t getDirectionX() const;
   [[nodiscard]] int32_t getDirectionY() const;

   void unmapBomb();
   void remapBomb();

   void reset();

   // inter-bomb collisions
   bool checkCollision(const BombKickAnimation& animation) const;
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
   Map& _map;

   // the kicked bomb, lifted off the map while it moves (see Map::addLiftedItem())
   int32_t _bomb_unique_id = -1;

   // field the bomb was kicked from
   Point _start_field;

   // player positions to collide with
   std::unordered_map<int32_t, Point> _player_positions;

   // run in the destructor, see addDestroyCallback()
   std::vector<std::function<void()>> _destroy_callbacks;

   [[nodiscard]] static uint64_t nextId();

   // key into the registry, animations are neither copyable nor movable
   uint64_t _id = nextId();

   // currently active kick animations, in creation order - each one registers itself in the
   // constructor and leaves in the destructor; the owner is the BombMapItem holding it
   static std::map<uint64_t, std::reference_wrapper<BombKickAnimation>> _animations;

   // bomb is colliding with another bomb
   bool _colliding = false;
};
