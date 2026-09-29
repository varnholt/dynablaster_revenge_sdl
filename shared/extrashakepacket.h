#pragma once

#include <cstdint>

#include "packet.h"

class ExtraShakePacket : public Packet
{
public:
   // write constructor
   ExtraShakePacket(int32_t unique_id);

   // read constructor
   ExtraShakePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getMapItemUniqueId() const;

private:
   int32_t _map_item_unique_id = 0;
};
