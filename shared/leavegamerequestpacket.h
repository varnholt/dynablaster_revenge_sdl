#pragma once

#include "packet.h"

class LeaveGameRequestPacket : public Packet
{
public:
   LeaveGameRequestPacket();
   LeaveGameRequestPacket(int32_t game_id, int32_t player_id);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   void setGameId(int32_t id);
   [[nodiscard]] int32_t getGameId() const;

   void setPlayerId(int32_t id);
   [[nodiscard]] int32_t getPlayerId() const;

protected:
   int32_t _game_id = -1;
   int32_t _player_id = -1;
};
