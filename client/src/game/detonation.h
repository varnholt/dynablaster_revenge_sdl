#pragma once

class Detonation
{
public:
   Detonation(int cx, int cy, int left, int right, int top, int bottom);

   int getX() const;
   int getY() const;
   int getUp() const;
   int getDown() const;
   int getLeft() const;
   int getRight() const;

   void setStartTime(float time);
   float elapsed(float time) const;

private:
   int _center_x;
   int _center_y;
   int _left;
   int _right;
   int _top;
   int _bottom;
   float _start_time = 0.0f;
};
