#pragma once

// base
#include "mapitemremovedpacket.h"

// constants
#include "constants.h"

class MapItemDestroyedPacket : public MapItemPacket
{
public:
   //! read constructor
   MapItemDestroyedPacket();

   //! write constructor
   MapItemDestroyedPacket(MapItem* item, int32_t playerId, Constants::Direction direction, float intensity);

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for destroyer id
   [[nodiscard]] int32_t getPlayerId() const;

   //! getter for direction
   [[nodiscard]] Constants::Direction getDirection() const;

   //! getter for intensity of destruction
   [[nodiscard]] float getIntensity() const;

private:
   //! player id
   int32_t mPlayerId;

   //! direction the item was destroyed from
   Constants::Direction mDirection;

   //! intensity the item was destroyed with
   float mIntensity;
};
