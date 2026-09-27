#pragma once

#include "packet.h"

class JoinGameRequestPacket : public Packet
{
public:
   //! write constructor
   JoinGameRequestPacket(int32_t id);

   //! read constructor
   JoinGameRequestPacket();

   //! destructor
   virtual ~JoinGameRequestPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for game id
   [[nodiscard]] int32_t getId() const;

private:
   //! game's id
   int32_t mId;
};
