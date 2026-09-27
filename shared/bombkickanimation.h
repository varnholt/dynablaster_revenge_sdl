#pragma once

// shared
#include "constants.h"
#include "point.h"
#include "signal.h"
#include "timer.h"

#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

// forward declarations
class BombMapItem;
class Map;
class MapItem;

class BombKickAnimation
{
public:
   //! constructor
   BombKickAnimation();

   //! destructor
   virtual ~BombKickAnimation();

   //! remove all animations
   static void deleteAll();

   //! run an arbitrary callback right before this animation is destroyed - used by whoever
   //! subscribed a Signal<> connected against a longer-lived signal (Game's own signals live
   //! for the whole match, this animation doesn't) to disconnect itself, since Signal<> has
   //! no automatic disconnect-on-destroy the way Qt's own connect() did
   void addDestroyCallback(std::function<void()> callback);

   //! start animation
   void start();

   //! setter for map
   void setMap(Map* map);

   //! setter for x position
   void setX(float x);

   //! setter for y position
   void setY(float y);

   //! getter for x position
   [[nodiscard]] float getX() const;

   //! getter for y position
   [[nodiscard]] float getY() const;

   //! getter for map
   [[nodiscard]] Map* getMap() const;

   //! check if ready to explode
   [[nodiscard]] bool isReadyToExplode() const;

   //! set bomb to "ready" to explode
   void setReadyToExplode(bool ready);

   //! setter for direction
   void setDirection(Constants::Direction dir);

   //! ignite animation at x, y
   static void ignite(int32_t x, int32_t y);

public:
   // Signal<> replacements for BombKickAnimation's former Qt signals (see
   // project_full_qt_removal_scope memory).

   //! bomb may explode now
   Signal<> explodeSignal;

   //! animation started
   Signal<Constants::Direction, float> startedSignal;

   //! animation stopped
   Signal<> stoppedSignal;

public:
   //! enable mReadyToExplode flag
   void readyToExplode();

   //! update a player position
   void updatePlayerPosition(int32_t id, float x, float y);

   //! remove player position if player died
   void removePlayerPosition(int32_t id);

protected:
   //! update the bomb's position
   void updatePosition();

protected:
   //! check if values are in range
   bool isInRange(float value1, float value2, float epsilon);

   //! movement may be continued
   bool isMoveAllowed();

   //! getter for step speed
   [[nodiscard]] float getStepSize() const;

   //! getter for direction
   [[nodiscard]] Constants::Direction getDirection() const;

   //! getter for x direction
   [[nodiscard]] int32_t getDirectionX() const;

   //! getter for y direction
   [[nodiscard]] int32_t getDirectionY() const;

   //! unmap bomb
   void unmapBomb();

   //! remap bomb
   void remapBomb();

   //! reset animation state
   void reset();

   // inter-bomb-collisions

   //! add animation to static list
   static void addAnimation(BombKickAnimation* animation);

   //! remove animation from static list
   static void removeAnimation(BombKickAnimation* animation);

   //! check if bomb collides with another bomb
   bool checkCollision(BombKickAnimation* animation);

   //! getter for colliding flag
   [[nodiscard]] bool isColliding() const;

   //! setter for colliding flag
   void setColliding(bool colliding);

   //! update collisions
   void updateCollisions();

   //! animation update timer
   Timer mTimer;

   //! intensity factor
   float mFactor;

   //! irection
   Constants::Direction mDirection;

   //! x position
   float mX;

   //! y position
   float mY;

   //! ready to explode flag
   bool mReadyToExplode;

   //! ptr to game
   Map* mMap;

   //! bomb map item
   BombMapItem* mBombMapItem;

   //! player positions to collide with
   std::unordered_map<int32_t, Point> mPlayerPositions;

   //! run in the destructor - see addDestroyCallback()
   std::vector<std::function<void()>> mDestroyCallbacks;

   // inter-bomb-collisions

   //! list of currently active kick animations - non-owning, self-registering tracker (each
   //! instance adds itself in the constructor, removes itself in the destructor), exactly like
   //! Timer::_timers; the real owner is whichever BombMapItem holds it via its
   //! std::unique_ptr<BombKickAnimation> mAnimation.
   //!
   //! TODO: deleteAll() below calls `delete` directly through this non-owning pointer. That is
   //! only safe if every BombMapItem that might still own one of these has already released or
   //! outlived it by the time deleteAll() runs - Game::~Game() currently calls deleteAll() BEFORE
   //! `delete mMap`, so any BombMapItem still mid-kick when a match ends will have its mAnimation
   //! unique_ptr double-delete an already-freed object once Map's destructor tears it down. Fixing
   //! this needs a teardown-order or ownership change in server/ (Game::~Game()/Map), outside
   //! shared/'s scope for this pass - flagged here, not fixed.
   static std::vector<BombKickAnimation*> sAnimations;

   //! bomb is colliding with another bomb
   bool mColliding;
};
