#pragma once

#include <cstdint>
#include <string>

#include "constants.h"
#include "creategamedata.h"
#include "packet.h"

class CreateGameRequestPacket : public Packet
{
public:
   // write constructor
   CreateGameRequestPacket(
      const std::string& name,
      const std::string& level,
      int32_t rounds,
      int32_t duration,
      int32_t max_players,
      bool extra_bomb_enabled,
      bool extra_flame_enabled,
      bool extra_speedup_enabled,
      bool extra_kick_enabled,
      bool extra_skulls_enabled,
      Constants::Dimension dimension
   );

   // read constructor
   CreateGameRequestPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] std::string getName() const;
   [[nodiscard]] CreateGameData getData() const;

   //! makes this a story mode game starting at the given stage
   void setStory(int32_t first_stage);

private:
   CreateGameData _data;
};
