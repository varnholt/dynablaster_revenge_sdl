#pragma once

// shared
#include "vec2.h"


class PlayerRotation
{

   public:

      //! constructor
      PlayerRotation();

      //! setter for angle increment 0..360
      static void setAngleIncrement(float angle);

      //! setter for target direction (position on the unit circle)
      void setTargetVector(const Vec2&);

      //! getter for the target vector
      [[nodiscard]] const Vec2& getTargetVector() const;

      //! update angle
      void updateAngle();

      //! getter for the current angle
      [[nodiscard]] float getAngle() const;

      //! getter for the previous angle
      [[nodiscard]] float getPreviousAngle() const;

      //! getter for angle delta
      [[nodiscard]] float getAngleDelta() const;

      //! reset angle
      void reset();


   private:

      //! angle to increment by each update
      static float mAngleIncrement;

      //! current angle
      float mAngle;

      //! previous angle
      float mPreviousAngle;

      //! target angle
      float mTargetAngle;

      //! delta from previous angle to current angle
      float mDelta;

      //! target vector
      Vec2 mTargetVector;

      //! down vector
      static Vec2 mDown;
};
