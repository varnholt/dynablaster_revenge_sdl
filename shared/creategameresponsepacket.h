#pragma once

#include "gameinformation.h"
#include "packet.h"

class CreateGameResponsePacket : public Packet
{
public:
   // write constructor
   CreateGameResponsePacket(const GameInformation& game_information);

   // read constructor
   CreateGameResponsePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] const GameInformation& getGameInformation() const;

private:
   GameInformation _game_information;
};
