#pragma once

#include <vector>

#include "mapitemcreatedpacket.h"

class ExtraMapItem;

class ExtraMapItemCreatedPacket : public MapItemCreatedPacket
{
public:
   // write constructor
   ExtraMapItemCreatedPacket(ExtraMapItem* item);

   // read constructor
   ExtraMapItemCreatedPacket();

   void enqueue(BinaryWriter& out) override;
   void dequeue(BinaryReader& in) override;

   [[nodiscard]] int32_t getExtraType() const;

   // the skull's six faces
   void setSkullFaces(const std::vector<Constants::SkullType>& faces);
   [[nodiscard]] std::vector<Constants::SkullType> getSkullFaces() const;

private:
   int32_t _extra_type = -1;
   std::vector<Constants::SkullType> _skull_faces = std::vector<Constants::SkullType>(6, Constants::SkullReset);
};
