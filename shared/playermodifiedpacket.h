#pragma once

#include "constants.h"
#include "packet.h"

class PlayerModifiedPacket : public Packet
{
public:
   // read constructor
   PlayerModifiedPacket();

   // write constructor
   explicit PlayerModifiedPacket(Constants::Color color);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] Constants::Color getColor() const;

private:
   int32_t _color = 0;
};
