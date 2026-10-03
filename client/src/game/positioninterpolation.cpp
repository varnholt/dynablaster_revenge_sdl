#include "positioninterpolation.h"

// cmath
#include <cmath>

// game
#include "bombermanclient.h"
#include "gamestatemachine.h"
#include "mapitemanimation.h"

// shared
#include "framework/globaltime.h"
#include "mapitem.h"

#include <algorithm>

#define MAP_ITEM_MOVE_EPSILON 0.05f

PositionInterpolation::PositionInterpolation()
{
   _timer.timeoutSignal.connect([this]() { update(); });

   GameStateMachine::getInstance().stateChangedSignal.connect([this]() { gameStateChanged(); });
}

PositionInterpolation::~PositionInterpolation() = default;

void PositionInterpolation::update()
{
   float cur_time = GlobalTime::Instance().getTime();

   float client_delta = cur_time - _time;

   // rescale client frame delta to server heartbeat interval
   float server_delta = 1.0f / SERVER_HEARTBEAT_IN_HZ;
   float delta = client_delta / server_delta;

   interpolatePlayerPositions(delta);
   interpolateMapItemPositions(delta);

   _time = cur_time;
}

void PositionInterpolation::moveMapItem(const MapItem& item, Constants::Direction dir, float speed, int nominal_x, int nominal_y)
{
   const int32_t id = item.getUniqueId();

   // animation stopped
   if (dir == Constants::DirectionUnknown)
   {
      auto iter = _map_item_animations.find(id);

      // relocate bomb map item at its nominal position
      if (iter != _map_item_animations.end())
      {
         MapItemAnimation& anim = *iter->second;
         anim._nominal_x = nominal_x;
         anim._nominal_y = nominal_y;
         anim._direction = dir;
      }
   }
   else
   {
      if (std::ranges::find(_map_items, id, &AnimatedItem::_id) != _map_items.end())
      {
         MapItemAnimation& animation = *_map_item_animations[id];
         animation.reset();
         animation._direction = dir;
         animation._speed = speed;
      }
      else
      {
         auto animation = std::make_unique<MapItemAnimation>(dir, speed);

         // pass bounce signals
         animation->bounce_signal.connect([this]() { bounceSignal(); });

         // store item data
         _map_items.push_back({id, item.getX(), item.getY()});
         _map_item_animations[id] = std::move(animation);
      }
   }
}

void PositionInterpolation::removeMapItem(const MapItem& item)
{
   const int32_t id = item.getUniqueId();

   auto it = std::ranges::find(_map_items, id, &AnimatedItem::_id);
   if (it != _map_items.end())
      _map_items.erase(it);

   _map_item_animations.erase(id);
}

void PositionInterpolation::interpolatePlayerPositions(float delta)
{
   auto& players = BombermanClient::getInstance().getPlayerInfoMap();

   float x = 0.0f;
   float y = 0.0f;
   float r = 0.0f;

   float dx = 0.0f;
   float dy = 0.0f;
   float dr = 0.0f;

   for (auto& [id, player] : players)
   {
      x = player.getX();
      y = player.getY();
      r = player.getAngle();

      dx = player.getDeltaX();
      dy = player.getDeltaY();
      dr = player.getAngleDelta();

      //      qDebug("interpolation %d (%f): %f, %f, %f ", dx,dy,dr);

      x += dx * delta;
      y += dy * delta;
      r += dr * delta;

      player.setPosition(x, y, r);

      setPlayerPositionSignal(player.getId(), x, y, r);

      setPlayerSpeedSignal(player.getId(), dx, dy, dr);
   }
}

void PositionInterpolation::interpolateMapItemPositions(float dt)
{
   for (const AnimatedItem& item : _map_items)
   {
      MapItemAnimation& animation = *_map_item_animations[item._id];

      animation.animate(dt);

      switch (animation._direction)
      {
         case Constants::DirectionUp:
            animation._y -= animation._speed * dt;
            break;
         case Constants::DirectionDown:
            animation._y += animation._speed * dt;
            break;
         case Constants::DirectionLeft:
            animation._x -= animation._speed * dt;
            break;
         case Constants::DirectionRight:
            animation._x += animation._speed * dt;
            break;
         default:
            break;
      }

      float x = item._x + animation._x;
      float y = item._y + animation._y;
      float z = animation._z;

      if (animation._direction == Constants::DirectionUnknown)
      {
         float diff_x = x - animation._nominal_x;
         float diff_y = y - animation._nominal_y;

         if ((std::fabs(diff_x) > MAP_ITEM_MOVE_EPSILON) || (std::fabs(diff_y) > MAP_ITEM_MOVE_EPSILON))
         {
            animation._x -= 0.1 * diff_x;
            animation._y -= 0.1 * diff_y;

            // stop bouncing
            float factor = animation._factor;
            factor -= 0.1f;
            animation._factor = std::max(factor, 0.0f);

            //            qDebug(
            //               "anim: %f, %f, diff: %f, %f, pos: %f, %f",
            //               animation._x,
            //               animation._y,
            //               diffX,
            //               diffY,
            //               x,
            //               y
            //            );
         }
      }

      setMapItemPositionSignal(item._id, x, y, z);
   }
}

void PositionInterpolation::gameStateChanged()
{
   Constants::GameState state = GameStateMachine::getInstance().getState();

   switch (state)
   {
      case Constants::GamePreparing:
      {
         _timer.start(1);
         break;
      }

      case Constants::GameStopped:
      {
         // stop timer
         _timer.stop();

         // clear all animated mapitems
         _map_items.clear();
         _map_item_animations.clear();

         break;
      }

      default:
         break;
   }
}
