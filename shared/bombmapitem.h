#pragma once

// base
#include "mapitem.h"
#include "signal.h"

// shared
#include "timer.h"

#include <memory>

// forward declarations
class BombKickAnimation;
class Map;

class BombMapItem : public MapItem
{
public:
   //! detonation origin if bomb detonates passively
   enum DetonationOrigin
   {
      Active = 0,
      Left = 1,
      Right = 2,
      Top = 3,
      Bottom = 4
   };

   //! constructor
   BombMapItem(int32_t playerId, int32_t flames, int32_t id, int32_t x, int32_t y);

   //! destructor
   virtual ~BombMapItem();

   //! getter for player id
   [[nodiscard]] int8_t getPlayerId() const;

   //! getter for flames
   [[nodiscard]] int8_t getFlames() const;

   //! bomb is kicked
   void kick();

   //! setter for detonation origin
   void setDetonationOrigin(DetonationOrigin origin);

   //! getter for detonation origin
   [[nodiscard]] DetonationOrigin getDetonationOrigin() const;

   //! set detonation interval
   void setInterval(int32_t ms);

   //! get detonation interval
   [[nodiscard]] int32_t getInterval() const;

   //! setter for owner id
   void setPlayerId(int32_t id);

   //! getter for kicked flag
   [[nodiscard]] bool isKicked() const;

   //! setter for kicked flag
   void setKicked(bool kicked);

   //! setter for shadowed item
   void setShadowedItem(MapItem* shadowedItem);

   //! setter for igniter id
   void setIgniterId(int8_t id);

   //! getter for igniter id
   [[nodiscard]] int8_t getIgniterId() const;

   //! getter for shadowed item
   [[nodiscard]] MapItem* getShadowedItem();

   //! getter for kick animation
   [[nodiscard]] BombKickAnimation* getBombKickAnimation() const;

   //! setter for bomb kick animation - takes ownership
   void setBombKickAnimation(std::unique_ptr<BombKickAnimation> animation);

   //! setter for tick time
   static void setTickTime(int32_t time);

   //! getter for tick time
   [[nodiscard]] static int32_t getTickTime();

public:
   //! let the bomb explode now
   void stopTimer();

public:
   // Signal<> replacements for BombMapItem's former Qt signals (see
   // project_full_qt_removal_scope memory).

   //! bomb exploded
   Signal<BombMapItem*, bool> explodedSignal;

   //! kick animation was started or stopped
   Signal<Constants::Direction, float> kickAnimationSignal;

protected:
   //! active explosion
   void explodeActive();

   //! delayed explosion triggered from animation
   void explodeDelayed();

protected:
   //! bomb's timer
   Timer mTimer;

   //! bomb owner
   int8_t mPlayerId;

   //! bomb flames
   int8_t mFlames;

   //! kicked flag
   bool mKicked;

   //! bomb kick animation
   std::unique_ptr<BombKickAnimation> mAnimation;

   //! detonation origin
   DetonationOrigin mDetonationOrigin;

   //! mapitem that may be shadowed by a kicked bomb - raw, non-owning: the grid (Map) owns
   //! whatever this points at, if anything; whoever destroys a grid item is responsible for
   //! nulling out any bomb's mShadowedItem that observes it (see Game::bombExploded()'s
   //! cleanup loop, same explicit-invalidation pattern as Game::mSpectators)
   MapItem* mShadowedItem;

   //! tick time
   static int32_t sTickTime;

   //! bomb igniter
   int8_t mIgniterId;
};
