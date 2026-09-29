#pragma once

#include <cstdint>

#include "constants.h"
#include "packet.h"

class GameEventPacket : public Packet
{
public:
   enum GameEvent
   {
      Invalid,
      BombExploded,
      ExtraCollected,
      ExtraDestroyed
   };

   // write constructor
   GameEventPacket(GameEvent event, float intensity = 1.0f, int32_t x = -1, int32_t y = -1);

   // read constructor
   GameEventPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // bomb exploded
   [[nodiscard]] GameEvent getGameEvent() const;
   [[nodiscard]] float getIntensity() const;

   // extra collected
   void setPlayerId(int32_t player_id);
   void setExtraType(Constants::ExtraType extra_type);
   [[nodiscard]] int32_t getPlayerId() const;
   [[nodiscard]] Constants::ExtraType getExtraType() const;

   // affected field
   [[nodiscard]] int32_t getX() const;
   [[nodiscard]] int32_t getY() const;

private:
   // TODO: split into BombExplodedPacket (intensity) and ExtraCollectedPacket (player id, extra type)
   GameEvent _event = Invalid;
   float _intensity = 1.0f;
   int32_t _player_id = -1;
   Constants::ExtraType _extra_type = Constants::ExtraBomb;
   int32_t _x = -1;
   int32_t _y = -1;
};
