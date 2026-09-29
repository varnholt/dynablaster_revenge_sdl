#pragma once

class Vertex
{
public:
   Vertex() = default;
   Vertex(float x, float y, float u, float v);

   Vertex operator+(const Vertex& other) const;
   Vertex operator-(const Vertex& other) const;
   Vertex operator*(float f) const;

   float x = 0.0f;
   float y = 0.0f;
   float u = 0.0f;
   float v = 0.0f;
};
