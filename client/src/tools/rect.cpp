#include "rect.h"

Rect::Rect(float x1, float y1, float x2, float y2) : _min(x1, y1), _max(x2, y2), _valid(true)
{
}

Rect::Rect(const Vector2& min, const Vector2& max) : _min(min), _max(max), _valid(true)
{
}

float Rect::left() const
{
   return _min.x;
}

float Rect::right() const
{
   return _max.x;
}

float Rect::top() const
{
   return _min.y;
}

float Rect::bottom() const
{
   return _max.y;
}

float Rect::getWidth() const
{
   return getMax().x - getMin().x;
}

float Rect::getHeight() const
{
   return getMax().y - getMin().y;
}

void Rect::setMin(const Vector2& min)
{
   _min = min;
   _valid = true;
}

void Rect::setMax(const Vector2& max)
{
   _max = max;
   _valid = true;
}

const Vector2& Rect::getMin() const
{
   return _min;
}

const Vector2& Rect::getMax() const
{
   return _max;
}

bool Rect::valid() const
{
   return _valid;
}
