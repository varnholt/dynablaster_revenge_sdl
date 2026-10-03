// 2d vector class

#pragma once

#include "vector.h"

// do not derive from Streamable here!
// The pointer to the virtual function table will increase the size of "Vector" and we want it be exactly float[4]

class Stream;

class Vector2
{
#define EPS 0.0001f

public:
   float x, y;

   Vector2();
   Vector2(float x, float y = 0.0f);
   Vector2(Stream& stream);

   Vector2 operator+(const Vector2& other) const;  // add two vectors
   void operator+=(const Vector2& other);          // add another vector
   Vector2 operator-(const Vector2& other) const;  // subtract two vectors
   Vector2 operator-() const;                      // negate
   void operator-=(const Vector2& other);          // subtract another vector
   Vector2 operator*(const float scalar) const;    // multiply by scalar
   float operator*(const Vector2& other) const;    // dot product
   void operator*=(const float scalar);            // multiply by scalar

   void set(float x = 0.0f, float y = 0.0f);  // set components
   std::span<const float, 2> values() const;  // components as float[2]
   std::span<float, 2> values();
   Vector2 linear(const Vector2& other, float interpolation_factor) const;

   void operator<<(Stream& stream);  // stream operator
   void operator>>(Stream& stream);  // stream operator

   void load(Stream& stream);   // load components from stream
   void write(Stream& stream);  // write components to stream
};
