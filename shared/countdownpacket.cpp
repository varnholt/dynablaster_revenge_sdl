// header
#include "countdownpacket.h"

// Qt
#include "logging.h"

// defines
#define PACKETNAME "Countdown"

//-----------------------------------------------------------------------------
/*!
   write constructor

   \param name game's name
*/
CountdownPacket::CountdownPacket(int8_t countdown) : Packet(Packet::COUNTDOWN), mTimeLeft(countdown)
{
   mPacketName = PACKETNAME;
}

//-----------------------------------------------------------------------------
/*!
   read constructor
*/
CountdownPacket::CountdownPacket() : Packet(Packet::COUNTDOWN)
{
   mPacketName = PACKETNAME;
}

//-----------------------------------------------------------------------------
/*!
   destructor
*/
CountdownPacket::~CountdownPacket()
{
}

//-----------------------------------------------------------------------------
/*!
   \return time left
*/
int8_t CountdownPacket::getTimeLeft() const
{
   return mTimeLeft;
}

//-----------------------------------------------------------------------------
/*!
   \param out datastream to write members to
*/
void CountdownPacket::enqueue(BinaryWriter& out)
{
   // write members
   out << mTimeLeft;
}

//-----------------------------------------------------------------------------
/*!
   \param in datastream read members from
*/
void CountdownPacket::dequeue(BinaryReader& in)
{
   // read members
   in >> mTimeLeft;
}

//-----------------------------------------------------------------------------
/*!
   debug output of members
*/
void CountdownPacket::debug()
{
   // debug output login response
   qDebug("CountdownPacket:debug: time left: %d", mTimeLeft);
}
