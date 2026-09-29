#pragma once

#include <string>

#include "packet.h"

class LoginRequestPacket : public Packet
{
public:
   // write constructor
   LoginRequestPacket(const std::string& nick, bool bot);

   // read constructor
   LoginRequestPacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] const std::string& getNick() const;
   [[nodiscard]] bool isBot() const;

private:
   std::string _nick;
   bool _bot = false;
};
