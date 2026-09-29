// header
#include "collisiondetection.h"

// server
#include "game.h"

// shared
#include "logging.h"
#include "map.h"
#include "mapitem.h"
#include "player.h"

// stdlib
#include <algorithm>
#include <cmath>

namespace
{
constexpr float SERVER_CORNER_EPSILON_MINOR = 0.30f;
constexpr float SERVER_CORNER_EPSILON_MAJOR = 0.70f;
}  // namespace

void CollisionDetection::process(Player* player)
{
   int keys_pressed = player->getKeysPressed();
   bool moved = false;
   float desired_x_position = player->getX();
   float desired_y_position = player->getY();
   int8_t directions = 0;

   // ----------------------------------------------------------------------
   // drunken helli course correction (tm) mueslee 2012
   const bool up_pressed = keys_pressed & Constants::KeyUp;
   const bool down_pressed = keys_pressed & Constants::KeyDown;
   const bool left_pressed = keys_pressed & Constants::KeyLeft;
   const bool right_pressed = keys_pressed & Constants::KeyRight;

   const std::array<int32_t, 2> field = {
      static_cast<int32_t>(std::floor(player->getX())), static_cast<int32_t>(std::floor(player->getY()))
   };

   const std::array<int32_t, 2> left = {field[0] - 1, field[1]};
   const std::array<int32_t, 2> right = {field[0] + 1, field[1]};
   const std::array<int32_t, 2> up = {field[0], field[1] - 1};
   const std::array<int32_t, 2> down = {field[0], field[1] + 1};

   const bool field_left_allowed = !isFieldBlocked(left);
   const bool field_right_allowed = !isFieldBlocked(right);
   const bool field_up_allowed = !isFieldBlocked(up);
   const bool field_down_allowed = !isFieldBlocked(down);

   const float absolute_x = player->getX();
   const float absolute_y = player->getY();

   const float relative_x = absolute_x - std::floor(absolute_x);
   const float relative_y = absolute_y - std::floor(absolute_y);

   /*

             0.0, 0.0      1.0, 0.0      2.0, 0.0

   0.0, 0.0  +-------------+-------------+-------------+
             |             |             |             |
             |             |             |             |
             |             |             |             |
             |             |             |             |
             |             |             |             |
   0.0, 1.0  +---------<???+xxx>-----<xxx+???>---------+
             |             |/////////////| \
             |             |/////////////|  this part is handled (x)
             |             |/////////////|  1.5 ... 1.99999
             |             |/////////////|  => eventually check
             |             |/////////////|     2.0 ... 2.x, too
   0.0, 2.0  +-------------+-------------+     by adding 0.1?

   */

   if (down_pressed && !left_pressed && !right_pressed && !up_pressed && field_down_allowed && relative_x > SERVER_CORNER_EPSILON_MAJOR)
   {
      keys_pressed |= Constants::KeyLeft;
   }
   else if (down_pressed && !left_pressed && !right_pressed && !up_pressed && field_down_allowed &&
            relative_x < SERVER_CORNER_EPSILON_MINOR)
   {
      keys_pressed |= Constants::KeyRight;
   }
   else if (up_pressed && !left_pressed && !right_pressed && !down_pressed && field_up_allowed && relative_x > SERVER_CORNER_EPSILON_MAJOR)
   {
      keys_pressed |= Constants::KeyLeft;
   }
   else if (up_pressed && !left_pressed && !right_pressed && !down_pressed && field_up_allowed && relative_x < SERVER_CORNER_EPSILON_MINOR)
   {
      keys_pressed |= Constants::KeyRight;
   }
   else if (left_pressed && !right_pressed && !up_pressed && !down_pressed && field_left_allowed &&
            relative_y > SERVER_CORNER_EPSILON_MAJOR)
   {
      keys_pressed |= Constants::KeyUp;
   }
   else if (left_pressed && !right_pressed && !up_pressed && !down_pressed && field_left_allowed &&
            relative_y < SERVER_CORNER_EPSILON_MINOR)
   {
      keys_pressed |= Constants::KeyDown;
   }
   else if (right_pressed && !left_pressed && !up_pressed && !down_pressed && field_right_allowed &&
            relative_y > SERVER_CORNER_EPSILON_MAJOR)
   {
      keys_pressed |= Constants::KeyUp;
   }
   else if (right_pressed && !left_pressed && !up_pressed && !down_pressed && field_right_allowed &&
            relative_y < SERVER_CORNER_EPSILON_MINOR)
   {
      keys_pressed |= Constants::KeyDown;
   }

   // ----------------------------------------------------------------------

   updatePlayerDirections(player, keys_pressed, directions, desired_x_position, desired_y_position);

   // collision control
   int field_x = static_cast<int32_t>(std::floor(desired_x_position));
   int field_y = static_cast<int32_t>(std::floor(desired_y_position));
   float x_in_field = desired_x_position - std::floor(desired_x_position);
   float y_in_field = desired_y_position - std::floor(desired_y_position);
   float assigned_x_position = 0.0;
   float assigned_y_position = 0.0;
   const bool player_moves_vertically = (player->getY() != desired_y_position);
   const bool player_moves_horizontally = (player->getX() != desired_x_position);
   bool horizontal_movement_forbidden = false;
   bool vertical_movement_forbidden = false;

   bool horizontal_blocked = false;
   bool vertical_blocked = false;
   bool x_in_epsilon = false;
   bool y_in_epsilon = false;

   MapItem* kicked_bomb1 = nullptr;
   MapItem* kicked_bomb2 = nullptr;

   horizontal_blocked = isPositionBlocked(desired_x_position, desired_y_position, keys_pressed, true, field_x, field_y, &kicked_bomb1);

   vertical_blocked = isPositionBlocked(desired_x_position, desired_y_position, keys_pressed, false, field_x, field_y, &kicked_bomb2);

   x_in_field = desired_x_position - std::floor(desired_x_position);
   y_in_field = desired_y_position - std::floor(desired_y_position);

   x_in_epsilon = x_in_field <= 0.5 + SERVER_MOVE_EPSILON && x_in_field >= 0.5 - SERVER_MOVE_EPSILON;

   y_in_epsilon = y_in_field <= 0.5 + SERVER_MOVE_EPSILON && y_in_field >= 0.5 - SERVER_MOVE_EPSILON;

   // kick bombs if possible
   playerKicksBombSignal(player, kicked_bomb1, true, keys_pressed);
   playerKicksBombSignal(player, kicked_bomb2, false, keys_pressed);

   // ----------------------------------------------------------------------
   // experimental position correction

   if (!x_in_epsilon && !y_in_epsilon)
   {
      // retry horizontal
      desired_x_position = player->getX();
      desired_y_position = player->getY();

      int keys_fixed_horizontal = keys_pressed;

      if (keys_fixed_horizontal & Constants::KeyUp)
      {
         keys_fixed_horizontal &= ~(Constants::KeyUp);
      }
      if (keys_fixed_horizontal & Constants::KeyDown)
      {
         keys_fixed_horizontal &= ~(Constants::KeyDown);
      }

      updatePlayerDirections(player, keys_fixed_horizontal, directions, desired_x_position, desired_y_position);

      field_x = static_cast<int32_t>(std::floor(desired_x_position));
      field_y = static_cast<int32_t>(std::floor(desired_y_position));

      x_in_field = desired_x_position - std::floor(desired_x_position);
      y_in_field = desired_y_position - std::floor(desired_y_position);

      horizontal_blocked = isPositionBlocked(desired_x_position, desired_y_position, keys_fixed_horizontal, false, field_x, field_y);

      x_in_epsilon = x_in_field <= 0.5 + SERVER_MOVE_EPSILON && x_in_field >= 0.5 - SERVER_MOVE_EPSILON;

      y_in_epsilon = y_in_field <= 0.5 + SERVER_MOVE_EPSILON && y_in_field >= 0.5 - SERVER_MOVE_EPSILON;

      if (!x_in_epsilon && !y_in_epsilon)
      {
         desired_x_position = player->getX();
         desired_y_position = player->getY();

         int keys_fixed_vertical = keys_pressed;

         if (keys_fixed_vertical & Constants::KeyRight)
         {
            keys_fixed_vertical &= ~(Constants::KeyRight);
         }
         if (keys_fixed_vertical & Constants::KeyLeft)
         {
            keys_fixed_vertical &= ~(Constants::KeyLeft);
         }

         updatePlayerDirections(player, keys_fixed_vertical, directions, desired_x_position, desired_y_position);

         field_x = static_cast<int32_t>(std::floor(desired_x_position));
         field_y = static_cast<int32_t>(std::floor(desired_y_position));

         // check if vertical movement is now possible
         vertical_blocked = isPositionBlocked(desired_x_position, desired_y_position, keys_fixed_vertical, true, field_x, field_y);
      }

      x_in_field = desired_x_position - std::floor(desired_x_position);
      y_in_field = desired_y_position - std::floor(desired_y_position);

      // x should be in epsilon now
      x_in_epsilon = x_in_field <= 0.5 + SERVER_MOVE_EPSILON && x_in_field >= 0.5 - SERVER_MOVE_EPSILON;

      y_in_epsilon = y_in_field <= 0.5 + SERVER_MOVE_EPSILON && y_in_field >= 0.5 - SERVER_MOVE_EPSILON;
   }

   // ----------------------------------------------------------------------

   // allow vertical movement only when
   // - position is changed
   // - x position is within 0.5 +- tolerance
   // - desired position is not blocked
   if (player_moves_vertically && x_in_epsilon && !horizontal_blocked)
   {
      // correct position until the player reached the middle of a field
      // or move the player to its desired position
      horizontal_movement_forbidden = isPositionBlocked(desired_x_position, desired_y_position, keys_pressed, false, field_x, field_y);

      // x position is above the movement-path
      assigned_x_position =
         adjustXPosition(player, x_in_field, desired_x_position, player_moves_horizontally, horizontal_movement_forbidden);

      assigned_y_position = desired_y_position;

      // yup, player was moved
      moved = true;
   }

   // allow horizontal movement only when
   // - position is changed
   // - x position is within 0.5 +- tolerance
   // - desired position is not blocked
   if (player_moves_horizontally && y_in_epsilon && !vertical_blocked)
   {
      // correct position until the player reached the middle of a field
      // or move the player to its desired position
      vertical_movement_forbidden = isPositionBlocked(desired_x_position, desired_y_position, keys_pressed, true, field_x, field_y);

      // y position is above the movement-path
      assigned_y_position = adjustYPosition(player, y_in_field, desired_y_position, player_moves_vertically, vertical_movement_forbidden);

      assigned_x_position = desired_x_position;

      // yup, player was moved
      moved = true;
   }

   // update player rotation (if required)
   const bool rotation_changed = updateRotation(player, keys_pressed, moved, assigned_x_position, assigned_y_position);

   if (moved || rotation_changed)
   {
      const int skip_count = player->getPositionSkipCounter();

      if (!moved  // <- TODO: always set! why?
          || (getGame()->getState() != Constants::GameActive) || player->isKilled())
      {
         assigned_x_position = player->getX();
         assigned_y_position = player->getY();
      }

      if ((player->getKeysPressed() != player->getKeysPressedPreviously()) || (skip_count >= getGame()->getPositionSkipCount()))
      {
         playerMoveSignal(player, assigned_x_position, assigned_y_position, directions);
      }
      else
      {
         player->setPositionSkipCounter(skip_count + 1);
      }

      player->setX(assigned_x_position);
      player->setY(assigned_y_position);

      // notify kick animations about player positions
      if (!player->isKilled())
      {
         playerPositionChangedSignal(player->getId(), player->getX(), player->getY());
      }
   }
   else
   {
      playerIdleSignal(directions, player);
   }
}

void CollisionDetection::updatePlayerDirections(
   Player* player,
   int keys_pressed,
   int8_t& directions,
   float& desired_x_position,
   float& desired_y_position
)
{
   const float speed = player->getSpeed();

   if (keys_pressed & Constants::KeyUp)
   {
      desired_y_position = player->getY() - (SERVER_SPEED * speed);
      directions |= Constants::KeyUp;
   }

   if (keys_pressed & Constants::KeyDown)
   {
      desired_y_position = player->getY() + (SERVER_SPEED * speed);
      directions |= Constants::KeyDown;
   }

   if (keys_pressed & Constants::KeyLeft)
   {
      desired_x_position = player->getX() - (SERVER_SPEED * speed);
      directions |= Constants::KeyLeft;
   }

   if (keys_pressed & Constants::KeyRight)
   {
      desired_x_position = player->getX() + (SERVER_SPEED * speed);
      directions |= Constants::KeyRight;
   }
}

float CollisionDetection::adjustXPosition(
   Player* player,
   float x_in_field,
   float desired_x_position,
   bool player_moves_horizontally,
   bool horizontal_movement_forbidden
)
{
   float assigned_x_position = 0.0;
   const float speed = player->getSpeed();

   if (x_in_field > 0.5 && (horizontal_movement_forbidden || !player_moves_horizontally))
   {
      if (x_in_field - 0.5 < (SERVER_SPEED * speed))
      {
         assigned_x_position = std::floor(desired_x_position) + 0.5;
      }
      else
      {
         assigned_x_position = desired_x_position - (SERVER_SPEED * speed);
      }
   }

   // x position is below the movement-path
   else if (x_in_field < 0.5 && (horizontal_movement_forbidden || !player_moves_horizontally))
   {
      if (0.5 - x_in_field < (SERVER_SPEED * speed))
      {
         assigned_x_position = std::floor(desired_x_position) + 0.5;
      }
      else
      {
         assigned_x_position = desired_x_position + (SERVER_SPEED * speed);
      }
   }

   // x position is on the movement path
   else
   {
      assigned_x_position = desired_x_position;
   }

   return assigned_x_position;
}

float CollisionDetection::adjustYPosition(
   Player* player,
   float y_in_field,
   float desired_y_position,
   bool player_moves_vertically,
   bool vertical_movement_forbidden
)
{
   float assigned_y_position = 0.0;
   const float speed = player->getSpeed();

   if (y_in_field > 0.5 && (vertical_movement_forbidden || !player_moves_vertically))
   {
      if (y_in_field - 0.5 < (SERVER_SPEED * speed))
      {
         assigned_y_position = std::floor(desired_y_position) + 0.5;
      }
      else
      {
         assigned_y_position = desired_y_position - (SERVER_SPEED * speed);
      }
   }

   // y position is below the movement-path
   else if (y_in_field < 0.5 && (vertical_movement_forbidden || !player_moves_vertically))
   {
      if (0.5 - y_in_field < (SERVER_SPEED * speed))
      {
         assigned_y_position = std::floor(desired_y_position) + 0.5;
      }
      else
      {
         assigned_y_position = desired_y_position + (SERVER_SPEED * speed);
      }
   }

   // y position is on movement path
   else
   {
      assigned_y_position = desired_y_position;
   }

   return assigned_y_position;
}

bool CollisionDetection::isPositionBlocked(
   float x,
   float y,
   int keys_pressed,
   bool vertical_check,
   int field_x,
   int field_y,
   MapItem** blocking_item
)
{
   bool blocked = false;

   // add player bounds to desired positions
   if (vertical_check)
   {
      if (keys_pressed & Constants::KeyUp)
      {
         y -= 0.5;
      }

      if (keys_pressed & Constants::KeyDown)
      {
         y += 0.5;
      }
   }
   else
   {
      if (keys_pressed & Constants::KeyLeft)
      {
         x -= 0.5;
      }

      if (keys_pressed & Constants::KeyRight)
      {
         x += 0.5;
      }
   }

   // make field position of x and y
   const int field_x_position = static_cast<int32_t>(std::floor(x));
   const int field_y_position = static_cast<int32_t>(std::floor(y));

   // block if map bounds are exceeded
   if (field_x_position < 0 || field_y_position < 0 || field_x_position > getMap()->getWidth() - 1 ||
       field_y_position > getMap()->getHeight() - 1)
   {
      blocked = true;
   }

   // block if player hit block or stone
   else
   {
      MapItem* item = getMap()->getItem(field_x_position, field_y_position);

      if (item && item->isBlocking() && !(item->getX() == field_x && item->getY() == field_y))
      {
         if (blocking_item)
         {
            *blocking_item = item;
         }

         blocked = true;
      }
   }

   return blocked;
}

bool CollisionDetection::updateRotation(Player* player, int keys_pressed, bool moved, float assigned_x_position, float assigned_y_position)
{
   bool x_moved = false;
   bool y_moved = false;

   PlayerRotation* rotation = player->getPlayerRotation();
   const float previous_rotation_angle = rotation->getAngle();

   // init the new target vector depending either
   // on the player's keyboard inputs or - if no movement was allowed -
   // the previous target vector directions
   Vec2 direction;

   const bool cursor_keys_pressed = (keys_pressed & Constants::KeyUp) || (keys_pressed & Constants::KeyDown) ||
                                    (keys_pressed & Constants::KeyLeft) || (keys_pressed & Constants::KeyRight);

   if (keys_pressed & Constants::KeyUp)
   {
      direction.setY(1.0f);
   }

   if (keys_pressed & Constants::KeyDown)
   {
      direction.setY(-1.0f);
   }

   if (keys_pressed & Constants::KeyLeft)
   {
      direction.setX(-1.0f);
   }

   if (keys_pressed & Constants::KeyRight)
   {
      direction.setX(1.0f);
   }

   // only allow 90 degree rotations
   if (moved)
   {
      x_moved = assigned_x_position != player->getX();
      y_moved = assigned_y_position != player->getY();

      if (!x_moved)
      {
         direction.setX(0.0f);
      }

      if (!y_moved)
      {
         direction.setY(0.0f);
      }
   }

   // set new target vector and update the according angle
   if (cursor_keys_pressed || x_moved || y_moved)
   {
      player->getPlayerRotation()->setTargetVector(direction);
   }

   player->getPlayerRotation()->updateAngle();

   const float new_rotation_angle = rotation->getAngle();

   return std::abs(previous_rotation_angle - new_rotation_angle) * 100000.0f >
          std::min(std::abs(previous_rotation_angle), std::abs(new_rotation_angle));
}

bool CollisionDetection::isFieldBlocked(const std::array<int32_t, 2>& field, MapItem** blocking_item)
{
   const int x = field[0];
   const int y = field[1];

   bool blocked = false;

   // block if map bounds are exceeded
   if (x < 0 || y < 0 || x > getMap()->getWidth() - 1 || y > getMap()->getHeight() - 1)
   {
      blocked = true;
   }

   // block if player hit block or stone
   else
   {
      MapItem* item = getMap()->getItem(x, y);

      if (item && item->isBlocking())
      {
         if (blocking_item)
         {
            *blocking_item = item;
         }

         blocked = true;
      }
   }

   return blocked;
}

Game* CollisionDetection::getGame() const
{
   return _game;
}

void CollisionDetection::setGame(Game* game)
{
   _game = game;
}

Map* CollisionDetection::getMap() const
{
   return getGame()->getMap();
}
