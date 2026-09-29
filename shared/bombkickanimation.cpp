#include "bombkickanimation.h"

#include "bombmapitem.h"
#include "constants.h"
#include "logging.h"
#include "map.h"
#include "mapitem.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
constexpr float BOMB_MOVE_SPEED = 3.0f;
constexpr float BOMB_EXPLODE_EPSILON = 0.2f;

int32_t toField(float value)
{
   return static_cast<int32_t>(std::floor(value));
}
}  // namespace

std::vector<BombKickAnimation*> BombKickAnimation::_animations;

BombKickAnimation::BombKickAnimation() : _factor(BOMB_MOVE_SPEED)
{
   _timer.setInterval(1000 / SERVER_HEARTBEAT_IN_HZ);
   _timer.timeoutSignal.connect([this]() { updatePosition(); });

   addAnimation(this);
}

BombKickAnimation::~BombKickAnimation()
{
   for (const auto& callback : _destroy_callbacks)
   {
      callback();
   }

   removeAnimation(this);
}

void BombKickAnimation::addDestroyCallback(std::function<void()> callback)
{
   _destroy_callbacks.push_back(std::move(callback));
}

void BombKickAnimation::deleteAll()
{
   while (!_animations.empty())
   {
      delete _animations.front();
   }
}

void BombKickAnimation::start()
{
   reset();

   unmapBomb();

   if (!_timer.isActive())
   {
      _timer.start();
   }

   startedSignal(getDirection(), getStepSize());
}

void BombKickAnimation::setMap(Map* map)
{
   _map = map;
}

void BombKickAnimation::setX(float x)
{
   _x = x;
}

void BombKickAnimation::setY(float y)
{
   _y = y;
}

float BombKickAnimation::getX() const
{
   return _x;
}

float BombKickAnimation::getY() const
{
   return _y;
}

Map* BombKickAnimation::getMap() const
{
   return _map;
}

bool BombKickAnimation::isReadyToExplode() const
{
   return _ready_to_explode;
}

void BombKickAnimation::setReadyToExplode(bool ready)
{
   _ready_to_explode = ready;
}

void BombKickAnimation::setDirection(Constants::Direction direction)
{
   _direction = direction;
}

void BombKickAnimation::ignite(int32_t x, int32_t y)
{
   for (BombKickAnimation* animation : _animations)
   {
      if (toField(animation->getX()) == x && toField(animation->getY()) == y)
      {
         // use the setter instead of readyToExplode() since ignite() could be called more than
         // once - the bomb stays in 'can detonate'-state until it is in the middle of a field.
         animation->setReadyToExplode(true);
      }
   }
}

void BombKickAnimation::unmapBomb()
{
   const auto x = toField(_x);
   const auto y = toField(_y);

   _bomb_map_item = dynamic_cast<BombMapItem*>(getMap()->getItem(x, y));

   // either clear the field or re-set the shadowed item
   getMap()->setItem(x, y, _bomb_map_item->getShadowedItem());
}

void BombKickAnimation::remapBomb()
{
   const auto x = toField(_x);
   const auto y = toField(_y);

   _bomb_map_item->setX(x);
   _bomb_map_item->setY(y);

   // item may have shadowed an extra
   _bomb_map_item->setShadowedItem(getMap()->getItem(x, y));

   getMap()->setItem(x, y, _bomb_map_item);
}

void BombKickAnimation::reset()
{
   _ready_to_explode = false;
}

bool BombKickAnimation::isInRange(float value1, float value2, float epsilon)
{
   return std::fabs(value1 - value2) < epsilon;
}

Constants::Direction BombKickAnimation::getDirection() const
{
   return _direction;
}

int32_t BombKickAnimation::getDirectionX() const
{
   switch (getDirection())
   {
      case Constants::DirectionLeft:
         return -1;
      case Constants::DirectionRight:
         return 1;
      default:
         return 0;
   }
}

int32_t BombKickAnimation::getDirectionY() const
{
   switch (getDirection())
   {
      case Constants::DirectionUp:
         return -1;
      case Constants::DirectionDown:
         return 1;
      default:
         return 0;
   }
}

void BombKickAnimation::updatePosition()
{
   updateCollisions();

   if (isMoveAllowed() && !isColliding())
   {
      _x += getDirectionX() * getStepSize();
      _y += getDirectionY() * getStepSize();

      if (isReadyToExplode())
      {
         // check if bomb is finally in the center of a field so it can explode
         const float field_x = std::floor(_x);
         const float field_y = std::floor(_y);

         if (isInRange(std::fabs(_x - field_x), 0.5f, BOMB_EXPLODE_EPSILON) &&
             isInRange(std::fabs(_y - field_y), 0.5f, BOMB_EXPLODE_EPSILON))
         {
            remapBomb();
            explodeSignal();
         }
      }
   }
   else
   {
      remapBomb();

      // if bomb can't move tell client to stop the animation
      stoppedSignal();

      readyToExplode();

      // no more updates required, just wait until this instance is deleted when the bomb explodes
      _timer.stop();
   }
}

void BombKickAnimation::readyToExplode()
{
   // if bomb *is* already "ready to explode", then explode, otherwise wait for it to be in its
   // final position
   if (_ready_to_explode)
   {
      explodeSignal();
   }
   else
   {
      _ready_to_explode = true;
   }
}

void BombKickAnimation::updatePlayerPosition(int32_t id, float x, float y)
{
   _player_positions[id] = Point(toField(x), toField(y));
}

void BombKickAnimation::removePlayerPosition(int32_t id)
{
   _player_positions.erase(id);
}

bool BombKickAnimation::isMoveAllowed()
{
   bool allowed = true;

   const auto field_x = toField(getX());
   const auto field_y = toField(getY());

   int32_t check_field_x = 0;
   int32_t check_field_y = 0;

   switch (getDirection())
   {
      case Constants::DirectionLeft:
         check_field_x = toField(getX() - 0.5f - getStepSize());
         check_field_y = field_y;
         break;
      case Constants::DirectionRight:
         check_field_x = toField(getX() + 0.5f + getStepSize());
         check_field_y = field_y;
         break;
      case Constants::DirectionUp:
         check_field_x = field_x;
         check_field_y = toField(getY() - 0.5f - getStepSize());
         break;
      case Constants::DirectionDown:
         check_field_x = field_x;
         check_field_y = toField(getY() + 0.5f + getStepSize());
         break;
      default:
         break;
   }

   const auto player_at = [this](int32_t x, int32_t y)
   { return std::ranges::any_of(_player_positions, [x, y](const auto& entry) { return entry.second.x() == x && entry.second.y() == y; }); };

   // block if map bounds are exceeded or if bomb hit something
   if (check_field_x < 0 || check_field_y < 0 || check_field_x > getMap()->getWidth() - 1 || check_field_y > getMap()->getHeight() - 1)
   {
      allowed = false;
   }
   else
   {
      MapItem* item = getMap()->getItem(check_field_x, check_field_y);

      if (item && item->isBlocking())
      {
         allowed = false;
      }
      else
      {
         // if there is no blocking item, we still need to check for player collisions
         if (player_at(check_field_x, check_field_y))
         {
            allowed = false;
         }

         // of COURSE there is an exception.. when there's a player who picked up a couple of
         // speedups and he's running towards the bomb kick direction, bomb and player might just
         // overlap for the time of one server heartbeat. the result is simply a bomb moving
         // "through" a player.
         if (allowed)
         {
            // we do not do this for the field the bomb was kicked from, just when the bomb is
            // 'on its way'.
            const int32_t start_x = _bomb_map_item->getX();
            const int32_t start_y = _bomb_map_item->getY();

            // stop the animation if there's a player on the field the bomb is at *this very moment*
            if ((field_x != start_x || field_y != start_y) && player_at(field_x, field_y))
            {
               allowed = false;
            }
         }
      }
   }

   return allowed;
}

float BombKickAnimation::getStepSize() const
{
   return _factor * SERVER_SPEED;
}

void BombKickAnimation::addAnimation(BombKickAnimation* animation)
{
   _animations.push_back(animation);
}

void BombKickAnimation::removeAnimation(BombKickAnimation* animation)
{
   std::erase(_animations, animation);
}

bool BombKickAnimation::checkCollision(BombKickAnimation* animation)
{
   bool colliding = false;

   const float eps = 1.0f + 2.0f * getStepSize();
   bool check_vertical = false;
   bool check_horizontal = false;

   // concept for simple horizontal/vertical checks:
   // if they get near enough (distance in x or y below eps) then they're colliding. eps is greater
   // than 1 because it is necessary to ensure both collisions end on two different fields.
   switch (getDirection())
   {
      case Constants::DirectionUp:
         check_vertical = (animation->getDirection() == Constants::DirectionDown);
         break;

      case Constants::DirectionDown:
         check_vertical = (animation->getDirection() == Constants::DirectionUp);
         break;

      case Constants::DirectionLeft:
         check_horizontal = (animation->getDirection() == Constants::DirectionRight);
         break;

      case Constants::DirectionRight:
         check_horizontal = (animation->getDirection() == Constants::DirectionLeft);
         break;

      default:
         break;
   }

   if (check_vertical)
   {
      if (toField(getX()) == toField(animation->getX()) && std::fabs(getY() - animation->getY()) < eps)
      {
         colliding = true;
      }
   }
   else if (check_horizontal)
   {
      if (toField(getY()) == toField(animation->getY()) && std::fabs(getX() - animation->getX()) < eps)
      {
         colliding = true;
      }
   }

   // check diagonal collisions as well... yes, fuck murphy :)
   // concept for diagonal collision checks:
   // shift both running animations along their directions.
   // this shifting is done for each direction individually, i.e.
   // - shift both animation
   // - shift x or/and y of animation 1
   // - shift x or/and y of animation 2
   if (!colliding)
   {
      /*
         +---+---+         +---+
         |   |///|         |///|
         +---+---+ lu      +---+---+ ld
         |///|             |   |///|
         +---+             +---+---+


         +---+---+             +---+
         |///|   |             |///|
         +---+---+ ru      +---+---+ rd
             |///|         |///|   |
             +---+         +---+---+

         ---------------------------

         +---+             +---+---+
         |///|             |///|   |
         +---+---+ dl      +---+---+ ur
         |   |///|             |///|
         +---+---+             +---+


         +---+---+             +---+
         |   |///|             |///|
         +---+---+ ul      +---+---+ dr
         |///|             |///|   |
         +---+             +---+---+
      */

      const float diagonal_eps = 2.0f * getStepSize();

      float x1 = getX();
      float y1 = getY();
      float x2 = animation->getX();
      float y2 = animation->getY();

      const auto x_field1 = toField(getX());
      const auto y_field1 = toField(getY());
      const auto x_field2 = toField(animation->getX());
      const auto y_field2 = toField(animation->getY());

      float x1_add = 0.0f;
      float y1_add = 0.0f;
      float x2_add = 0.0f;
      float y2_add = 0.0f;

      // shift positions into possible collision range
      const auto direction = getDirection();
      const auto other_direction = animation->getDirection();

      // lu
      if (direction == Constants::DirectionLeft && other_direction == Constants::DirectionUp)
      {
         x1_add = -diagonal_eps;
         y2_add = -diagonal_eps;
      }
      // ld
      else if (direction == Constants::DirectionLeft && other_direction == Constants::DirectionDown)
      {
         x1_add = -diagonal_eps;
         y2_add = diagonal_eps;
      }
      // ru
      else if (direction == Constants::DirectionRight && other_direction == Constants::DirectionUp)
      {
         x1_add = diagonal_eps;
         y2_add = -diagonal_eps;
      }
      // rd
      else if (direction == Constants::DirectionRight && other_direction == Constants::DirectionDown)
      {
         x1_add = diagonal_eps;
         y2_add = diagonal_eps;
      }
      // dl
      else if (direction == Constants::DirectionDown && other_direction == Constants::DirectionLeft)
      {
         y1_add = diagonal_eps;
         x2_add = -diagonal_eps;
      }
      // ul
      else if (direction == Constants::DirectionUp && other_direction == Constants::DirectionLeft)
      {
         y1_add = -diagonal_eps;
         x2_add = -diagonal_eps;
      }
      // ur
      else if (direction == Constants::DirectionUp && other_direction == Constants::DirectionRight)
      {
         y1_add = -diagonal_eps;
         x2_add = diagonal_eps;
      }
      // dr
      else if (direction == Constants::DirectionDown && other_direction == Constants::DirectionRight)
      {
         y1_add = diagonal_eps;
         x2_add = diagonal_eps;
      }

      x1 += x1_add;
      y1 += y1_add;
      x2 += x2_add;
      y2 += y2_add;

      const auto x_field_shifted1 = toField(x1);
      const auto y_field_shifted1 = toField(y1);
      const auto x_field_shifted2 = toField(x2);
      const auto y_field_shifted2 = toField(y2);

      // check if fields are in collision range
      colliding = (x_field_shifted1 == x_field_shifted2 && y_field_shifted1 == y_field_shifted2)  // all shifted
                  || (x_field_shifted1 == x_field2 && y_field_shifted1 == y_field2)               // animation 1 shifted
                  || (x_field_shifted1 == x_field2 && y_field1 == y_field2)                       // animation 1 x shifted
                  || (x_field1 == x_field2 && y_field_shifted1 == y_field2)                       // animation 1 y shifted
                  || (x_field1 == x_field_shifted2 && y_field1 == y_field_shifted2)               // animation 2 shifted
                  || (x_field1 == x_field_shifted2 && y_field1 == y_field2)                       // animation 2 x shifted
                  || (x_field1 == x_field2 && y_field1 == y_field_shifted2);                      // animation 2 y shifted
   }

   return colliding;
}

bool BombKickAnimation::isColliding() const
{
   return _colliding;
}

void BombKickAnimation::setColliding(bool colliding)
{
   _colliding = colliding;
}

void BombKickAnimation::updateCollisions()
{
   // check our current animation vs. all animations that are currently active
   for (BombKickAnimation* animation : _animations)
   {
      // if the other kicked bomb collides with this kicked bomb, set both to 'colliding' ->
      // they're remapped in the next step so they're stopped and placed next to each other.
      if (animation != this && checkCollision(animation))
      {
         setColliding(true);
         animation->setColliding(true);
      }
   }
}
