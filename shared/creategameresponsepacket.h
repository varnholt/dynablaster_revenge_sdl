#pragma once

// shared
#include "gameinformation.h"
#include "packet.h"

// Qt

class CreateGameResponsePacket : public Packet
{
public:
   //! write constructor
   CreateGameResponsePacket(const GameInformation& gameInformation);

   //! read constructor
   CreateGameResponsePacket();

   //! destructor
   virtual ~CreateGameResponsePacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for game information
   [[nodiscard]] const GameInformation& getGameInformation() const;

private:
   //! game information
   GameInformation mGameInformation;
};
