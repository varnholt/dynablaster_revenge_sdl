#pragma once

#include <vector>

#include "gameinformation.h"
#include "packet.h"

class ListGamesResponsePacket : public Packet
{
public:
   // write constructor
   ListGamesResponsePacket(const std::vector<GameInformation>& games, bool update = false);

   // read constructor
   ListGamesResponsePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] std::vector<GameInformation> getGames() const;

   // update flag: the packet updates already known game information
   void setUpdate(bool update);
   [[nodiscard]] bool isUpdate() const;

private:
   std::vector<GameInformation> _games;
   bool _update = false;
};
