#ifndef MAPITEMANIMATION_H
#define MAPITEMANIMATION_H

// shared
#include "constants.h"
#include "signal.h"


class MapItemAnimation
{
   public:

      MapItemAnimation(
         Constants::Direction dir,
         float speed
      );

      void animate(float dt);
      void reset();

      Constants::Direction mDirection;
      float mSpeed;
      float mX;
      float mY;
      float mZ;
      float mZPrev;
      int mNominalX;
      int mNominalY;
      float mTime;
      bool mTick;
      float mFactor;

      Signal<> bounceSignal;
};

#endif // MAPITEMANIMATION_H
