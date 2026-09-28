#include "positioninterpolation.h"

// cmath
#include <cmath>

// game
#include "bombermanclient.h"
#include "gamestatemachine.h"
#include "mapitemanimation.h"

// shared
#include "mapitem.h"
#include "framework/globaltime.h"

#include <algorithm>

#define MAP_ITEM_MOVE_EPSILON 0.05f


PositionInterpolation::PositionInterpolation()
{
   mTimer.timeoutSignal.connect([this]() { update(); });

   GameStateMachine::getInstance()->stateChangedSignal.connect([this]() { gameStateChanged(); });
}


PositionInterpolation::~PositionInterpolation() = default;


void PositionInterpolation::update()
{
   float curTime= GlobalTime::Instance()->getTime();

   float clientDelta= curTime - mTime;

   // rescale client frame delta to server heartbeat interval
   float serverDelta= 1.0f / SERVER_HEARTBEAT_IN_HZ;
   float delta= clientDelta / serverDelta;

   interpolatePlayerPositions(delta);
   interpolateMapItemPositions(delta);

   mTime= curTime;
}


void PositionInterpolation::moveMapItem(
   MapItem * item,
   Constants::Direction dir,
   float speed,
   int nominalX,
   int nominalY
)
{
   // animation stopped
   if (dir == Constants::DirectionUnknown)
   {
      auto iter = mMapItemAnimations.find(item);

      // relocate bomb map item at its nominal position
      if (iter != mMapItemAnimations.end())
      {
         MapItemAnimation* anim = iter->second.get();
         anim->_nominal_x = nominalX;
         anim->_nominal_y = nominalY;
         anim->_direction = dir;
      }
   }
   else
   {
      MapItemAnimation* animation = 0;

      if (std::find(mMapItems.begin(), mMapItems.end(), item) != mMapItems.end())
      {
         animation = mMapItemAnimations[item].get();
         animation->reset();
         animation->_direction = dir;
         animation->_speed = speed;
      }
      else
      {
         auto owned_animation = std::make_unique<MapItemAnimation>(dir, speed);
         animation = owned_animation.get();

         // pass bounce signals
         animation->bounce_signal.connect([this]() { bounceSignal(); });

         // store item data
         mMapItems.push_back(item);
         mMapItemAnimations[item] = std::move(owned_animation);
      }
   }
}


void PositionInterpolation::removeMapItem(MapItem* item)
{
   auto it = std::find(mMapItems.begin(), mMapItems.end(), item);
   if (it != mMapItems.end())
      mMapItems.erase(it);

   auto animIt = mMapItemAnimations.find(item);
   if (animIt != mMapItemAnimations.end())
   {
      mMapItemAnimations.erase(animIt);
   }
}


void PositionInterpolation::interpolatePlayerPositions(float delta)
{
   const auto* players =
      BombermanClient::getInstance()->getPlayerInfoMap();

   float x = 0.0f;
   float y = 0.0f;
   float r = 0.0f;

   float dx = 0.0f;
   float dy = 0.0f;
   float dr = 0.0f;

   for (const auto& [id, player] : *players)
   {
      x = player->getX();
      y = player->getY();
      r = player->getAngle();

      dx = player->getDeltaX();
      dy = player->getDeltaY();
      dr = player->getAngleDelta();

//      qDebug("interpolation %d (%f): %f, %f, %f ", dx,dy,dr);

      x+= dx*delta;
      y+= dy*delta;
      r+= dr*delta;

      player->setPosition(
         x,
         y,
         r
      );

      setPlayerPositionSignal(
         player->getId(),
         x,
         y,
         r
      );

      setPlayerSpeedSignal(
         player->getId(),
         dx,
         dy,
         dr
      );
   }
}


void PositionInterpolation::interpolateMapItemPositions(float dt)
{
   for (MapItem* item : mMapItems)
   {
      MapItemAnimation* animation = mMapItemAnimations[item].get();

      animation->animate(dt);

      switch (animation->_direction)
      {
         case Constants::DirectionUp:
            animation->_y -= animation->_speed*dt;
            break;
         case Constants::DirectionDown:
            animation->_y += animation->_speed*dt;
            break;
         case Constants::DirectionLeft:
            animation->_x -= animation->_speed*dt;
            break;
         case Constants::DirectionRight:
            animation->_x += animation->_speed*dt;
            break;
         default:
            break;
      }

      float x = item->getX() + animation->_x;
      float y = item->getY() + animation->_y;
      float z = animation->_z;

      if (animation->_direction == Constants::DirectionUnknown)
      {
         float diffX = x - animation->_nominal_x;
         float diffY = y - animation->_nominal_y;

         if (
               (std::fabs(diffX) > MAP_ITEM_MOVE_EPSILON)
            || (std::fabs(diffY) > MAP_ITEM_MOVE_EPSILON)
         )
         {
            animation->_x -= 0.1 * diffX;
            animation->_y -= 0.1 * diffY;

            // stop bouncing
            float factor = animation->_factor;
            factor -= 0.1f;
            animation->_factor = std::max(factor, 0.0f);

//            qDebug(
//               "anim: %f, %f, diff: %f, %f, pos: %f, %f",
//               animation->_x,
//               animation->_y,
//               diffX,
//               diffY,
//               x,
//               y
//            );
         }
      }

      setMapItemPositionSignal(
         item,
         x,
         y,
         z
      );
   }
}


void PositionInterpolation::gameStateChanged()
{
   Constants::GameState state = GameStateMachine::getInstance()->getState();

   switch (state)
   {
      case Constants::GamePreparing:
      {
         mTimer.start(1);
         break;
      }

      case Constants::GameStopped:
      {
         // stop timer
         mTimer.stop();

         // clear all animated mapitems
         mMapItems.clear();
         mMapItemAnimations.clear();

         break;
      }

      default:
         break;
   }
}

