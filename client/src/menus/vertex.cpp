#include "vertex.h"

Vertex::Vertex(float x, float y, float u, float v) : x(x), y(y), u(u), v(v)
{
}

Vertex Vertex::operator+(const Vertex& other) const
{
   return Vertex(x + other.x, y + other.y, u + other.u, v + other.v);
}

Vertex Vertex::operator-(const Vertex& other) const
{
   return Vertex(x - other.x, y - other.y, u - other.u, v - other.v);
}

Vertex Vertex::operator*(float f) const
{
   return Vertex(x * f, y * f, u * f, v * f);
}
