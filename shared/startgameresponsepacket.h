#pragma once

#include "packet.h"

// Qt

class StartGameResponsePacket : public Packet
{
public:
   //! write constructor, -1 if not accepted
   StartGameResponsePacket(int32_t id, bool started);

   //! read constructor
   StartGameResponsePacket();

   //! destructor
   virtual ~StartGameResponsePacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for game id
   [[nodiscard]] int32_t getId() const;

   //! getter for game is started flag
   [[nodiscard]] bool isStarted() const;

private:
   //! game id
   int32_t mId;

   //! game was started
   bool mStarted;
};
