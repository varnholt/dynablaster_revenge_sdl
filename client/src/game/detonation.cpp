#include "detonation.h"

Detonation::Detonation(int cx, int cy, int left, int right, int top, int bottom)
    : _center_x(cx),
      _center_y(cy),
      _left(left),
      _right(right),
      _top(top),
      _bottom(bottom)
{
}

int Detonation::getX() const
{
   return _center_x;
}

int Detonation::getY() const
{
   return _center_y;
}

int Detonation::getUp() const
{
   return _top;
}

int Detonation::getDown() const
{
   return _bottom;
}

int Detonation::getLeft() const
{
   return _left;
}

int Detonation::getRight() const
{
   return _right;
}

void Detonation::setStartTime(float time)
{
   _start_time= time;
}

float Detonation::elapsed(float time) const
{
   return time - _start_time;
}
