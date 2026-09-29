#pragma once

#include "math/vector2.h"

class Rect
{
public:
   Rect() = default;
   Rect(float x1, float y1, float x2, float y2);
   Rect(const Vector2& min, const Vector2& max);

   float left() const;
   float right() const;
   float top() const;
   float bottom() const;
   float getWidth() const;
   float getHeight() const;

   bool valid() const;
   void setMin(const Vector2& min);
   void setMax(const Vector2& max);

   const Vector2& getMin() const;
   const Vector2& getMax() const;

protected:
   Vector2 _min{0.0f, 0.0f};
   Vector2 _max{0.0f, 0.0f};
   bool _valid = false;
};
