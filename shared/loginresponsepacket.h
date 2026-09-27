#pragma once

#include <string>

// base
#include "packet.h"

// shared
#include "serverconfiguration.h"

class LoginResponsePacket : public Packet
{
public:
   //! write constructor
   LoginResponsePacket(bool broadcasted, int32_t id, const std::string& nick, const ServerConfiguration& serverConfig);

   //! read constructor
   LoginResponsePacket();

   //! destructor
   virtual ~LoginResponsePacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! setter for the id
   void setId(int32_t id);

   //! getter for the id
   [[nodiscard]] int32_t getId() const;

   //! getter for player nick
   [[nodiscard]] const std::string& getNick() const;

   //! getter for broadcast flag
   [[nodiscard]] bool isBroadcast() const;

   //! getter for server configuration
   [[nodiscard]] const ServerConfiguration& getServerConfiguration() const;

private:
   //! individual allowance or broadcast packet
   bool mBroadcast;

   //! player's
   int32_t mId;

   //! player nick
   std::string mNick;

   //! server configuration data
   ServerConfiguration mServerConfiguration;
};
