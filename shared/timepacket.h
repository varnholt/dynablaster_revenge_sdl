#pragma once

// base
#include "packet.h"

// constants
#include "constants.h"

class TimePacket : public Packet
{
public:
   //! read constructor
   TimePacket();

   //! write constructor
   TimePacket(int32_t timeLeft);

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for elapsed time
   [[nodiscard]] int32_t getTimeLeft() const;

private:
   //! elapsed time in ms
   int32_t mTimeLeft;
};
