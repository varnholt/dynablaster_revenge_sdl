#pragma once

#include "constants.h"
#include "packet.h"

class PlayerKilledPacket : public Packet
{
public:
   // read constructor
   PlayerKilledPacket();

   // write constructor
   PlayerKilledPacket(int32_t player_id, int32_t player_killed_by_id, Constants::Direction direction, float intensity);

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getPlayerId() const;

private:
   int32_t _player_id = 0;
   int32_t _player_killed_by_id = 0;

   // direction the player was killed from
   Constants::Direction _direction = Constants::DirectionUp;

   // intensity the player was killed with
   float _intensity = 0.0f;
};
