#pragma once

#include <string>

#include "packet.h"
#include "serverconfiguration.h"

class LoginResponsePacket : public Packet
{
public:
   // write constructor
   LoginResponsePacket(bool broadcast, int32_t id, const std::string& nick, const ServerConfiguration& server_configuration);

   // read constructor
   LoginResponsePacket();

   void debug() override;
   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   // player id, -1 if the login was denied
   void setId(int32_t id);
   [[nodiscard]] int32_t getId() const;

   [[nodiscard]] const std::string& getNick() const;

   // individual response or broadcast to every client
   [[nodiscard]] bool isBroadcast() const;

   [[nodiscard]] const ServerConfiguration& getServerConfiguration() const;

private:
   bool _broadcast = false;
   int32_t _id = -1;
   std::string _nick;
   ServerConfiguration _server_configuration;
};
