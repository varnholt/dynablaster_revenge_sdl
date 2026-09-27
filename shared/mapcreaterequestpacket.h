#pragma once

#include <cstdint>

// base
#include "packet.h"

#include "point.h"

#include <vector>

class MapCreateRequestPacket : public Packet
{
public:
   //! write constructor
   MapCreateRequestPacket(
      int32_t width,
      int32_t height,
      int32_t stoneCount,
      int32_t extraBombCount,
      int32_t extraFlameCount,
      const std::vector<Point>& startPositions
   );

   //! read constructor
   MapCreateRequestPacket();

   //! destructor
   virtual ~MapCreateRequestPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

private:
   //! map width
   int32_t mWidth;

   //! map height
   int32_t mHeight;

   //! number of stones
   int32_t mStoneCount;

   //! number of bomb extras
   int32_t mExtraBombCount;

   //! number of flame extras
   int32_t mExtraFlameCount;

   //! player start positions
   std::vector<Point> mStartPositions;
};
