#pragma once

#include "constants.h"
#include "packet.h"

#include <optional>

class JoinGameRequestPacket : public Packet
{
public:
   // write constructor
   explicit JoinGameRequestPacket(int32_t id, std::optional<Constants::Color> preferred_color = std::nullopt);

   // read constructor
   JoinGameRequestPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // game id
   [[nodiscard]] int32_t getId() const;

   // the color the player would like to have, if it's still free
   [[nodiscard]] std::optional<Constants::Color> getPreferredColor() const;

private:
   int32_t _id = 0;
   std::optional<Constants::Color> _preferred_color;
};
