#pragma once

#include "gamesignal.h"
#include "mapitem.h"
#include "timer.h"

#include <memory>

class BombKickAnimation;

class BombMapItem : public MapItem
{
public:
   // detonation origin if bomb detonates passively
   enum DetonationOrigin
   {
      Active = 0,
      Left = 1,
      Right = 2,
      Top = 3,
      Bottom = 4
   };

   BombMapItem(int32_t player_id, int32_t flames, int32_t id, int32_t x, int32_t y);
   ~BombMapItem() override;

   [[nodiscard]] int8_t getPlayerId() const;
   [[nodiscard]] int8_t getFlames() const;

   void kick();

   void setDetonationOrigin(DetonationOrigin origin);
   [[nodiscard]] DetonationOrigin getDetonationOrigin() const;

   // detonation interval
   void setInterval(int32_t ms);
   [[nodiscard]] int32_t getInterval() const;

   // owner id
   void setPlayerId(int32_t id);

   [[nodiscard]] bool isKicked() const;
   void setKicked(bool kicked);

   // item hidden underneath the bomb, empty if none
   void setShadowedItem(std::shared_ptr<MapItem> shadowed_item);
   [[nodiscard]] const std::shared_ptr<MapItem>& getShadowedItem() const;
   [[nodiscard]] std::shared_ptr<MapItem> takeShadowedItem();

   void setIgniterId(int8_t id);
   [[nodiscard]] int8_t getIgniterId() const;

   [[nodiscard]] bool hasBombKickAnimation() const;
   [[nodiscard]] BombKickAnimation& getBombKickAnimation() const;

   // takes ownership
   void setBombKickAnimation(std::unique_ptr<BombKickAnimation> animation);

   static void setTickTime(int32_t time);
   [[nodiscard]] static int32_t getTickTime();

   // let the bomb explode now
   void stopTimer();

   Signal<BombMapItem&, bool> explodedSignal;

   // kick animation was started or stopped
   Signal<Constants::Direction, float> kickAnimationSignal;

protected:
   void explodeActive();

   // delayed explosion triggered from animation
   void explodeDelayed();

   Timer _timer;

   // bomb owner
   int8_t _player_id = 0;

   int8_t _flames = 0;
   bool _kicked = false;
   std::unique_ptr<BombKickAnimation> _animation;
   DetonationOrigin _detonation_origin = Active;

   // item that may be shadowed by a kicked bomb, off the grid while it is covered
   std::shared_ptr<MapItem> _shadowed_item;

   static int32_t _tick_time;

   int8_t _igniter_id = -1;
};
