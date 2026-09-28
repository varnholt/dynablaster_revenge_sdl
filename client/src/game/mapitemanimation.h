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
      float _x;
      float _y;
      float _z;
      float _z_prev;
      int _nominal_x;
      int _nominal_y;
      float _time;
      bool _tick;
      float _factor;

      Signal<> bounce_signal;
};

#endif // MAPITEMANIMATION_H
