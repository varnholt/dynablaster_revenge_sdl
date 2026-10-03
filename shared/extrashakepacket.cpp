#include "extrashakepacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "ExtraShake";
}

ExtraShakePacket::ExtraShakePacket(int32_t unique_id) : Packet(Packet::EXTRASHAKE), _map_item_unique_id(unique_id)
{
   _packet_name = PACKETNAME;
}

ExtraShakePacket::ExtraShakePacket() : Packet(Packet::EXTRASHAKE)
{
   _packet_name = PACKETNAME;
}

int32_t ExtraShakePacket::getMapItemUniqueId() const
{
   return _map_item_unique_id;
}

void ExtraShakePacket::enqueue(BinaryWriter& out)
{
   out << _map_item_unique_id;
}

void ExtraShakePacket::dequeue(BinaryReader& in)
{
   in >> _map_item_unique_id;
}

void ExtraShakePacket::debug()
{
   qDebug("ExtraShakePacket: unique id: %d", _map_item_unique_id);
}
