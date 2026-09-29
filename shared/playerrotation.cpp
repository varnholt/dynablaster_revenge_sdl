#include "playerrotation.h"

#include <algorithm>
#include <cmath>
#include <numbers>

float PlayerRotation::_angle_increment = 0.2f;
Vec2 PlayerRotation::_down = Vec2(0.0f, -1.0f);

void PlayerRotation::setAngleIncrement(float angle)
{
   _angle_increment = angle;
}

void PlayerRotation::setTargetVector(const Vec2& target)
{
   _target_vector = target;

   /*
      the player may only move in 4 directions:
      - right
      - down
      - left
      - up

      => the player rotation is interpolated between these values.


                        |
                        | 0,1
                        |
                    _.-"+"-._
                  .'    |    `.
                 /      |      \
                |       |       |
         -------+-------+-------+-------
         -1,0   |       |       |   1,0
                 \      |      /
                  `._   |   _.'
                     `-.+.-'
                        |
                        | 0,-1
                        |

   */

   // map the atan2 circle to 0..2*PI
   _target_angle = std::atan2(-target.y(), -target.x()) + std::numbers::pi_v<float>;
}

const Vec2& PlayerRotation::getTargetVector() const
{
   return _target_vector;
}

void PlayerRotation::updateAngle()
{
   /*
         |--------+<<<<<<<<<<<<<<<<<<<<<<+------------|
         0        |                      |           360
                  |                      |
                  to                    from

         aaaaaaaaa|bbbbbbbbbbbbbbbbbbbbbb|aaaaaaaaaaaaa


         case 1: from > to

            a = 360 - from + to
            b = from - to

         |--------+>>>>>>>>>>>>>>>>>>>>>>+------------|
         0        |                      |           360
                  |                      |
                 from                    to

         aaaaaaaaa|bbbbbbbbbbbbbbbbbbbbbb|aaaaaaaaaaaaa

         case 2:  to > from

            a = from + 360 - to
            b = to - from

         increase = (a > b)
   */

   constexpr auto two_pi = 2.0f * std::numbers::pi_v<float>;

   float a = 0.0f;
   float b = 0.0f;

   const float from = _angle;
   const float to = _target_angle;

   if (from >= to)
   {
      a = from - to;
      b = to + two_pi - from;
   }
   else
   {
      a = from + two_pi - to;
      b = to - from;
   }

   const bool increase = (a > b);

   _previous_angle = _angle;

   if (increase)
   {
      const float delta = std::min(_angle_increment, b);

      _delta = delta;
      _angle += delta;

      if (_angle >= two_pi)
      {
         _angle -= two_pi;
      }
   }
   else
   {
      const float delta = std::min(_angle_increment, a);

      _delta = -delta;
      _angle -= delta;

      if (_angle < 0.0f)
      {
         _angle += two_pi;
      }

      if (_angle < _target_angle && _previous_angle >= _target_angle)
      {
         _angle = _target_angle;
      }
   }
}

float PlayerRotation::getAngle() const
{
   return _angle;
}

float PlayerRotation::getPreviousAngle() const
{
   return _previous_angle;
}

float PlayerRotation::getAngleDelta() const
{
   return _delta;
}

void PlayerRotation::reset()
{
   setTargetVector(_down);
}
