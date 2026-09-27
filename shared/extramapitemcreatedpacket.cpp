
// header
#include "extramapitemcreatedpacket.h"

// shared
#include "extramapitem.h"

// Qt

// defines
#define PACKETNAME "ExtraMapItemCreated"

//-----------------------------------------------------------------------------
/*!
   \param item extra-mapitem
*/
ExtraMapItemCreatedPacket::ExtraMapItemCreatedPacket(ExtraMapItem* item)
    : MapItemCreatedPacket(item), mExtraType(item->getExtraType()), mSkullFaces(6, Constants::SkullReset)
{
   mPacketType = Packet::EXTRAMAPITEMCREATED;
   mPacketName = PACKETNAME;
}

//-----------------------------------------------------------------------------
/*!
   read constructor
*/
ExtraMapItemCreatedPacket::ExtraMapItemCreatedPacket() : MapItemCreatedPacket(), mExtraType(-1), mSkullFaces(6, Constants::SkullReset)
{
   mPacketType = Packet::EXTRAMAPITEMCREATED;
   mPacketName = PACKETNAME;
}

//-----------------------------------------------------------------------------
/*!
   \return the extra map item's type
*/
int32_t ExtraMapItemCreatedPacket::getExtraType() const
{
   return mExtraType;
}

//-----------------------------------------------------------------------------
/*!
   \param sides skull sides
*/
void ExtraMapItemCreatedPacket::setSkullFaces(const std::vector<Constants::SkullType>& faces)
{
   mSkullFaces = faces;
}

//-----------------------------------------------------------------------------
/*!
   \return enabled skull sides
*/
std::vector<Constants::SkullType> ExtraMapItemCreatedPacket::getSkullFaces() const
{
   return mSkullFaces;
}

//-----------------------------------------------------------------------------
/*!
   \param out datastream to write members to
*/
void ExtraMapItemCreatedPacket::enqueue(BinaryWriter& out)
{
   uint32_t faces = (mSkullFaces[0] << 20) | (mSkullFaces[1] << 16) | (mSkullFaces[2] << 12) | (mSkullFaces[3] << 8) |
                    (mSkullFaces[4] << 4) | (mSkullFaces[5]);

   MapItemCreatedPacket::enqueue(out);

   // write members
   out << mExtraType;
   out << faces;
}

//-----------------------------------------------------------------------------
/*!
   \param in datastream read members from
*/
void ExtraMapItemCreatedPacket::dequeue(BinaryReader& in)
{
   MapItemCreatedPacket::dequeue(in);

   // read members
   int32_t extraType = 0;
   uint32_t faces = 0;

   in >> extraType;
   in >> faces;

   mExtraType = static_cast<Constants::ExtraType>(extraType);
   mSkullFaces[0] = static_cast<Constants::SkullType>((faces >> 20) & 0x0f);
   mSkullFaces[1] = static_cast<Constants::SkullType>((faces >> 16) & 0x0f);
   mSkullFaces[2] = static_cast<Constants::SkullType>((faces >> 12) & 0x0f);
   mSkullFaces[3] = static_cast<Constants::SkullType>((faces >> 8) & 0x0f);
   mSkullFaces[4] = static_cast<Constants::SkullType>((faces >> 4) & 0x0f);
   mSkullFaces[5] = static_cast<Constants::SkullType>((faces) & 0x0f);
}
