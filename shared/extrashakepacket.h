#pragma once

#include <cstdint>

#include "packet.h"

class ExtraShakePacket : public Packet
{
public:
   //! write constructor
   ExtraShakePacket(int32_t uniqueId);

   //! read constructor
   ExtraShakePacket();

   //! destructor
   virtual ~ExtraShakePacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for item unique id
   [[nodiscard]] int32_t getMapItemUniqueId() const;

private:
   //! unique id
   int32_t mMapItemUniqueId;
};
