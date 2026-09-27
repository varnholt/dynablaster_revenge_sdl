// header
#include "mapitemmovepacket.h"

// Qt
#include "logging.h"

// defines
#define PACKETNAME "MapItemMove"

//-----------------------------------------------------------------------------
/*!
   write constructor

   \param pId player id
   \param xPos player x position
   \param yPos player y position
   \param angle optional rotation angle
*/
MapItemMovePacket::MapItemMovePacket(int32_t id, float speed, Constants::Direction dir, int32_t nominalX, int32_t nominalY)
    : Packet(Packet::MAPITEMMOVE), mMapItemId(id), mSpeed(speed), mDirection(dir), mNominalX(nominalX), mNominalY(nominalY)
{
   mPacketName = PACKETNAME;
}

//-----------------------------------------------------------------------------
/*!
   read constructor
*/
MapItemMovePacket::MapItemMovePacket()
    : Packet(Packet::MAPITEMMOVE), mMapItemId(0), mSpeed(0.0), mDirection(Constants::DirectionUnknown), mNominalX(0), mNominalY(0)
{
   mPacketName = PACKETNAME;
}

//-----------------------------------------------------------------------------
/*!
   destructor
*/
MapItemMovePacket::~MapItemMovePacket()
{
}

//-----------------------------------------------------------------------------
/*!
   \return player x position
*/
float MapItemMovePacket::getSpeed() const
{
   return mSpeed;
}

//-----------------------------------------------------------------------------
/*!
   \return mapitem direction
*/
Constants::Direction MapItemMovePacket::getDirection() const
{
   return mDirection;
}

//-----------------------------------------------------------------------------
/*!
   \return nominal x
*/
int32_t MapItemMovePacket::getNominalX() const
{
   return mNominalX;
}

//-----------------------------------------------------------------------------
/*!
   \return nominal y
*/
int32_t MapItemMovePacket::getNominalY() const
{
   return mNominalY;
}

//-----------------------------------------------------------------------------
/*!
   \return player id
*/
int32_t MapItemMovePacket::getMapItemId() const
{
   return mMapItemId;
}

//-----------------------------------------------------------------------------
/*!
   \param out datastream to write members to
*/
void MapItemMovePacket::enqueue(BinaryWriter& out)
{
   // write player id
   out << mMapItemId;
   out << mSpeed;
   out << static_cast<int8_t>(mDirection);
   out << mNominalX;
   out << mNominalY;
}

//-----------------------------------------------------------------------------
/*!
   \param in datastream read members from
*/
void MapItemMovePacket::dequeue(BinaryReader& in)
{
   int8_t direction = 0;

   in >> mMapItemId >> mSpeed >> direction >> mNominalX >> mNominalY;

   mDirection = static_cast<Constants::Direction>(direction);
}

//-----------------------------------------------------------------------------
/*!
   debug output of members
*/
void MapItemMovePacket::debug()
{
   // output player id and x, y
   qDebug("MapItemMovePacket: unique id: %d, speed: %f, nominal: %d, %d", mMapItemId, mSpeed, mNominalX, mNominalY);
}
