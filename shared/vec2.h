#pragma once

class Vec2
{
public:
   Vec2() = default;
   Vec2(float x, float y);

   [[nodiscard]] float x() const;
   [[nodiscard]] float y() const;

   void setX(float x);
   void setY(float y);

   [[nodiscard]] float length() const;

   [[nodiscard]] Vec2 operator-(const Vec2& other) const;

private:
   float _x = 0.0f;
   float _y = 0.0f;
};
