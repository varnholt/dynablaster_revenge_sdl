#include "stopgameresponsepacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "StopGameResponse";
}

StopGameResponsePacket::StopGameResponsePacket(int32_t id, bool finished) : Packet(Packet::STOPGAMERESPONSE), _id(id), _finished(finished)
{
   _packet_name = PACKETNAME;
}

StopGameResponsePacket::StopGameResponsePacket() : Packet(Packet::STOPGAMERESPONSE)
{
   _packet_name = PACKETNAME;
}

int32_t StopGameResponsePacket::getId() const
{
   return _id;
}

bool StopGameResponsePacket::isFinished() const
{
   return _finished;
}

void StopGameResponsePacket::enqueue(BinaryWriter& out)
{
   out << _id << _finished;
}

void StopGameResponsePacket::dequeue(BinaryReader& in)
{
   in >> _id >> _finished;
}

void StopGameResponsePacket::debug()
{
   qDebug("StopGameResponsePacket:debug: id: %d, all rounds finished: %d", _id, _finished);
}
