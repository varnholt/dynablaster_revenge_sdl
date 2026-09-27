#pragma once

// base
#include "packet.h"

// constants
#include "constants.h"

class PlayerModifiedPacket : public Packet
{
public:
   //! read constructor
   PlayerModifiedPacket();

   //! write constructor
   PlayerModifiedPacket(Constants::Color color);

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for elapsed time
   [[nodiscard]] Constants::Color getColor() const;

private:
   //! player color
   int32_t mColor;
};
