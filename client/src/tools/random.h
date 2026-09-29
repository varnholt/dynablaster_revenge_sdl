#pragma once

#include <cstdlib>

inline constexpr float invRange = 1.0f / 16383.0f;

inline float frand(float max)
{
   return (std::rand() & 16383) * invRange * max;
}

inline float frands(float min, float max)
{
   return min + (std::rand() & 16383) * invRange * (max - min);
}
