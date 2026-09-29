#pragma once

#include <string>

#include "constants.h"
#include "packet.h"

class JoinGameResponsePacket : public Packet
{
public:
   // write constructor
   JoinGameResponsePacket(bool success, int32_t game_id, int32_t player_id, const std::string& nick, Constants::Color color);

   // read constructor
   JoinGameResponsePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   void setGameId(int32_t id);
   [[nodiscard]] int32_t getGameId() const;

   void setPlayerId(int32_t id);
   [[nodiscard]] int32_t getPlayerId() const;

   [[nodiscard]] const std::string& getNick() const;
   [[nodiscard]] bool isSuccessful() const;

   void setColor(Constants::Color color);
   [[nodiscard]] Constants::Color getColor() const;

private:
   bool _success = false;
   int32_t _game_id = -1;
   int32_t _player_id = -1;
   std::string _nick;
   Constants::Color _color = Constants::ColorWhite;
};
