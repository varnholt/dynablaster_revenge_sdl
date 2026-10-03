#pragma once

#include <cstdint>
#include "light.h"

class Stream;

class Spot : public Light
{
public:
   Spot(Node* parent = nullptr);
   void load(Stream& stream) override;
   void write(Stream& stream) override;

private:
   int32_t _shape = 0;
   float _hotspot = 0.0f;
   float _fall_size = 0.0f;
};
