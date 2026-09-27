#pragma once

#include "packet.h"

class CountdownPacket : public Packet
{
public:
   //! write constructor
   CountdownPacket(int8_t timeLeft);

   //! read constructor
   CountdownPacket();

   //! destructor
   virtual ~CountdownPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for remaining time
   [[nodiscard]] int8_t getTimeLeft() const;

private:
   //! countdown
   int8_t mTimeLeft;
};
