#pragma once

#include "packet.h"

// Qt

class StopGameResponsePacket : public Packet
{
public:
   //! write constructor, -1 if not accepted
   StopGameResponsePacket(int32_t id, bool finished);

   //! read constructor
   StopGameResponsePacket();

   //! destructor
   virtual ~StopGameResponsePacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for game id
   [[nodiscard]] int32_t getId() const;

   //! getter for stopped flag
   [[nodiscard]] bool isFinished() const;

private:
   //! game id
   int32_t mId;

   //! finished flag
   bool mFinished;
};
