#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

// integer grid coordinate, mirroring QPoint's accessor shape
class Point
{
public:
   Point() = default;

   Point(int32_t x, int32_t y);

   [[nodiscard]] int32_t x() const;
   [[nodiscard]] int32_t y() const;

   void setX(int32_t x);
   void setY(int32_t y);

   [[nodiscard]] bool operator==(const Point& other) const;
   [[nodiscard]] bool operator!=(const Point& other) const;

   [[nodiscard]] Point operator-(const Point& other) const;
   [[nodiscard]] Point operator-() const;

   [[nodiscard]] bool isNull() const;
   [[nodiscard]] int32_t manhattanLength() const;

private:
   int32_t _x = 0;
   int32_t _y = 0;
};

template <>
struct std::hash<Point>
{
   // boost-style hash_combine of the two components
   std::size_t operator()(const Point& point) const noexcept
   {
      std::size_t h1 = std::hash<int32_t>{}(point.x());
      std::size_t h2 = std::hash<int32_t>{}(point.y());
      return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
   }
};
