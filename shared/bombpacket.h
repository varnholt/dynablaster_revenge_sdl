#pragma once

#include "packet.h"

class BombPacket : public Packet
{
public:
   //! write constructor
   BombPacket(int8_t playerId, uint8_t x, uint8_t y);

   //! read constructor
   BombPacket();

   //! destructor
   virtual ~BombPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for player id
   [[nodiscard]] int8_t getPlayerId() const;

   //! getter for the bomb's x field position
   [[nodiscard]] uint8_t getX() const;

   //! getter for the bomb's y field position
   [[nodiscard]] uint8_t getY() const;

private:
   //! player id
   int8_t playerId;

   //! the bomb's x field position
   uint8_t x;

   //! the bomb's x field position
   uint8_t y;
};
