#pragma once

#include "packet.h"

// Qt

class StartGameRequestPacket : public Packet
{
public:
   //! write constructor
   StartGameRequestPacket(int32_t id);

   //! read constructor
   StartGameRequestPacket();

   //! destructor
   virtual ~StartGameRequestPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for game id
   [[nodiscard]] int32_t getId() const;

private:
   //! game id
   int32_t mId;
};
