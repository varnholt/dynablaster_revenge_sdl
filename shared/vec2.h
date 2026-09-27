#pragma once

class Vec2
{

public:

   //! default constructor, zero-initialized
   Vec2();

   Vec2(float x, float y);

   [[nodiscard]] float x() const;
   [[nodiscard]] float y() const;

   void setX(float x);
   void setY(float y);

   [[nodiscard]] float length() const;

   [[nodiscard]] Vec2 operator-(const Vec2& other) const;


private:

   float _x;
   float _y;
};
