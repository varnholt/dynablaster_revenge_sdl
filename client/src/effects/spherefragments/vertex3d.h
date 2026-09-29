#pragma once

#include "math/vector.h"

// interleaved vbo layout, uploaded as-is
struct Vertex3D
{
   Vector position;
   Vector normal;
   float u = 0.0f;
   float v = 0.0f;
   float index = 0.0f;
   float blend = 0.0f;
   Vector tangent;
};

static_assert(sizeof(Vertex3D) == 13 * sizeof(float));
