#pragma once

#include "packet.h"

class DetonationPacket : public Packet
{
public:
   // read constructor
   DetonationPacket();

   // write constructor
   DetonationPacket(int32_t x, int32_t y, int8_t fields_up, int8_t fields_down, int8_t fields_left, int8_t fields_right, float intensity);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // detonation center
   [[nodiscard]] int32_t getX() const;
   [[nodiscard]] int32_t getY() const;

   // number of fields the flames reach in each direction
   [[nodiscard]] int8_t getUp() const;
   [[nodiscard]] int8_t getDown() const;
   [[nodiscard]] int8_t getLeft() const;
   [[nodiscard]] int8_t getRight() const;

   // number of flames / flame intensity
   [[nodiscard]] float getIntensity() const;

private:
   int32_t _x = 0;
   int32_t _y = 0;
   int8_t _fields_up = 0;
   int8_t _fields_down = 0;
   int8_t _fields_left = 0;
   int8_t _fields_right = 0;
   float _intensity = 0.0f;
};
