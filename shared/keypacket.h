#pragma once

// base
#include "packet.h"

// shared
#include "constants.h"

class KeyPacket : public Packet
{
public:
   //! read constructor
   KeyPacket();

   //! write constructor
   KeyPacket(int8_t playerId, int8_t keys);

   //! destructor
   virtual ~KeyPacket();

   //! enqueue member variables
   void enqueue(BinaryWriter&);

   //! dequeue member variables
   void dequeue(BinaryReader&);

   //! getter for key combination
   [[nodiscard]] int8_t getKeys() const;

   //! getter for player id
   [[nodiscard]] int8_t getPlayerId() const;

   //! debug function
   void debug();

private:
   //! player id
   int8_t playerId;

   //! key combination
   int8_t keys;
};
