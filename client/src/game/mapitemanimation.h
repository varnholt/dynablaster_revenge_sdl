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

      Constants::Direction _direction;
      float _speed;
      float _x = 0.0f;
      float _y = 0.0f;
      float _z = 0.0f;
      float _z_prev = 0.0f;
      int _nominal_x = -1;
      int _nominal_y = -1;
      float _time = 0.0f;
      bool _tick = false;
      float _factor = 1.0f;

      Signal<> bounce_signal;
};

#endif // MAPITEMANIMATION_H
