#pragma once

#include <cstdint>

// base
#include "packet.h"

// shared
#include "constants.h"

class MapItemMovePacket : public Packet
{
public:
   //! write constructor
   MapItemMovePacket(int32_t mapItemId, float speed, Constants::Direction direction, int32_t nominalX = -1, int32_t nominalY = -1);

   //! read constructor
   MapItemMovePacket();

   //! destructor
   virtual ~MapItemMovePacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for object id
   [[nodiscard]] int32_t getMapItemId() const;

   //! getter mapitem speed
   [[nodiscard]] float getSpeed() const;

   //! getter for mapitem direction
   [[nodiscard]] Constants::Direction getDirection() const;

   //! getter for nominal x position
   [[nodiscard]] int32_t getNominalX() const;

   //! getter for nominal y position
   [[nodiscard]] int32_t getNominalY() const;

private:
   //! unique mapitem id
   int32_t mMapItemId;

   //! mapitem speed
   float mSpeed;

   //! mapitem direction
   Constants::Direction mDirection;

   //! nominal x
   int32_t mNominalX;

   //! nominal y
   int32_t mNominalY;
};
