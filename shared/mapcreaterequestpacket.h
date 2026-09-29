#pragma once

#include <cstdint>
#include <vector>

#include "packet.h"
#include "point.h"

class MapCreateRequestPacket : public Packet
{
public:
   // write constructor
   MapCreateRequestPacket(
      int32_t width,
      int32_t height,
      int32_t stone_count,
      int32_t extra_bomb_count,
      int32_t extra_flame_count,
      const std::vector<Point>& start_positions
   );

   // read constructor
   MapCreateRequestPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

private:
   int32_t _width = 0;
   int32_t _height = 0;
   int32_t _stone_count = 0;
   int32_t _extra_bomb_count = 0;
   int32_t _extra_flame_count = 0;
   std::vector<Point> _start_positions;
};
