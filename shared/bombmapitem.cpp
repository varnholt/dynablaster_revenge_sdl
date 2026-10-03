#include "bombmapitem.h"

#include "bombkickanimation.h"

int32_t BombMapItem::_tick_time = 0;

BombMapItem::BombMapItem(int32_t player_id, int32_t flames, int32_t id, int32_t x, int32_t y)
    : MapItem(Bomb, id, true, false, x, y), _player_id(static_cast<int8_t>(player_id)), _flames(static_cast<int8_t>(flames))
{
   _timer.timeoutSignal.connect([this]() { explodeActive(); });
   _timer.start(getTickTime());
}

BombMapItem::~BombMapItem()
{
   // the animation is safe to destroy synchronously here - this never runs nested inside the
   // animation's own signal dispatch (unlike explodeDelayed()), only from deferred teardown
   _timer.stop();
}

int8_t BombMapItem::getPlayerId() const
{
   return _player_id;
}

int8_t BombMapItem::getFlames() const
{
   return _flames;
}

void BombMapItem::explodeActive()
{
   if (!_kicked)
   {
      explodedSignal(*this, false);
   }
}

void BombMapItem::stopTimer()
{
   _timer.stop();
}

void BombMapItem::kick()
{
   // only connect these signals once
   if (!isKicked())
   {
      _animation->startedSignal.connect([this](Constants::Direction direction, float speed) { kickAnimationSignal(direction, speed); });

      _animation->stoppedSignal.connect([this]() { kickAnimationSignal(Constants::DirectionUnknown, 0.0f); });

      // notify animation when bomb exploded - reads _animation live so it observes a null
      // animation safely once explodeDelayed() has moved it out
      _timer.timeoutSignal.connect(
         [this]()
         {
            if (_animation)
            {
               _animation->readyToExplode();
            }
         }
      );

      // and notify map item back when animation reached the center of a field
      _animation->explodeSignal.connect([this]() { explodeDelayed(); });

      setKicked(true);
   }

   _animation->setX(getX() + 0.5f);
   _animation->setY(getY() + 0.5f);
   _animation->start();
}

void BombMapItem::explodeDelayed()
{
   explodedSignal(*this, false);

   if (isKicked() && _animation)
   {
      // this runs from inside the animation's own explodeSignal dispatch, so destroying it here
      // would destroy it while one of its methods is on the call stack - defer to the next tick.
      // it has nothing left to do, so it must not move the bomb again until then.
      // shared_ptr since std::function needs a copyable target.
      _animation->stop();
      Timer::singleShot(0, [animation = std::shared_ptr<BombKickAnimation>(std::move(_animation))]() {});
   }
}

void BombMapItem::setDetonationOrigin(DetonationOrigin origin)
{
   _detonation_origin = origin;
}

BombMapItem::DetonationOrigin BombMapItem::getDetonationOrigin() const
{
   return _detonation_origin;
}

void BombMapItem::setInterval(int32_t ms)
{
   _timer.setInterval(ms);
}

int32_t BombMapItem::getInterval() const
{
   return _timer.interval();
}

void BombMapItem::setPlayerId(int32_t id)
{
   _player_id = static_cast<int8_t>(id);
}

bool BombMapItem::isKicked() const
{
   return _kicked;
}

void BombMapItem::setKicked(bool kicked)
{
   _kicked = kicked;
}

void BombMapItem::setShadowedItem(std::shared_ptr<MapItem> shadowed_item)
{
   _shadowed_item = std::move(shadowed_item);
}

const std::shared_ptr<MapItem>& BombMapItem::getShadowedItem() const
{
   return _shadowed_item;
}

std::shared_ptr<MapItem> BombMapItem::takeShadowedItem()
{
   return std::move(_shadowed_item);
}

void BombMapItem::setIgniterId(int8_t id)
{
   _igniter_id = id;
}

int8_t BombMapItem::getIgniterId() const
{
   return _igniter_id;
}

bool BombMapItem::hasBombKickAnimation() const
{
   return _animation != nullptr;
}

BombKickAnimation& BombMapItem::getBombKickAnimation() const
{
   return *_animation;
}

void BombMapItem::setBombKickAnimation(std::unique_ptr<BombKickAnimation> animation)
{
   _animation = std::move(animation);
}

void BombMapItem::setTickTime(int32_t time)
{
   _tick_time = time;
}

int32_t BombMapItem::getTickTime()
{
   return _tick_time;
}
