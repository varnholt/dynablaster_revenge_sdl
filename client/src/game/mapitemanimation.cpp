#include "mapitemanimation.h"


// cmath
#include <cmath>

// defines
#define TICK_UPPER_LIMIT 0.1
#define TICK_LOWER_LIMIT 0.02


MapItemAnimation::MapItemAnimation(
   Constants::Direction dir,
   float speed
)
 : _direction(dir),
   _speed(speed),
   _x(0.0f),
   _y(0.0f),
   _z(0.0f),
   _z_prev(0.0f),
   _nominal_x(-1),
   _nominal_y(-1),
   _time(0.0f),
   _tick(false),
   _factor(1.0f)
{
}


void MapItemAnimation::animate(float dt)
{
   _z_prev = _z;

   // let the bomb bounce
   _time += dt * 0.1f;
   _z = std::fabs(std::sin(0.7f * std::pow(_time, 1.5f)) / (1.0f + _time)) * 1.5f;
   _z *= _factor;

   // check for floor tick sound
   if (
         _z     < TICK_UPPER_LIMIT
      && _z_prev > _z
   )
   {
      if (!_tick)
      {
         bounce_signal();
         _tick = true;
      }
   }
   else
   {
      if (_z > TICK_LOWER_LIMIT)
         _tick = false;
   }
}


void MapItemAnimation::reset()
{
   _time = 0.0f;
   _factor = 1.0f;
}
