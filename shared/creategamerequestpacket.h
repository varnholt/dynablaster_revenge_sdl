#pragma once

#include <cstdint>

// base
#include "packet.h"

// shared
#include "constants.h"
#include "creategamedata.h"

#include <string>

class CreateGameRequestPacket : public Packet
{
public:
   //! write constructor
   CreateGameRequestPacket(
      const std::string& name,
      const std::string& level,
      int32_t rounds,
      int32_t duration,
      int32_t maxPlayers,
      bool extraBombEnabled,
      bool extraFlameEnabled,
      bool extraSpeedupEnabled,
      bool extraKickEnabled,
      bool extraSkullsEnabled,
      Constants::Dimension dimension
   );

   //! read constructor
   CreateGameRequestPacket();

   //! destructor
   virtual ~CreateGameRequestPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for game name
   [[nodiscard]] std::string getName() const;

   //! getter for create game data
   [[nodiscard]] CreateGameData getData() const;

private:
   CreateGameData mData;
};
