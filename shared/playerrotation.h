#pragma once

#include "vec2.h"

class PlayerRotation
{
public:
   PlayerRotation() = default;

   // angle to increment by each update
   static void setAngleIncrement(float angle);

   // target direction (position on the unit circle)
   void setTargetVector(const Vec2& target);
   [[nodiscard]] const Vec2& getTargetVector() const;

   void updateAngle();

   [[nodiscard]] float getAngle() const;
   [[nodiscard]] float getPreviousAngle() const;

   // delta from previous angle to current angle
   [[nodiscard]] float getAngleDelta() const;

   void reset();

private:
   static float _angle_increment;

   float _angle = 0.0f;
   float _previous_angle = 0.0f;
   float _target_angle = 0.0f;
   float _delta = 0.0f;
   Vec2 _target_vector;

   static Vec2 _down;
};
