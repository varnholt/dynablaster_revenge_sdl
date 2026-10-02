#include "joingamerequestpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "JoinGameRequest";
}

JoinGameRequestPacket::JoinGameRequestPacket(int32_t id, std::optional<Constants::Color> preferred_color)
    : Packet(Packet::JOINGAMEREQUEST), _id(id), _preferred_color(preferred_color)
{
   _packet_name = PACKETNAME;
}

JoinGameRequestPacket::JoinGameRequestPacket() : Packet(Packet::JOINGAMEREQUEST)
{
   _packet_name = PACKETNAME;
}

int32_t JoinGameRequestPacket::getId() const
{
   return _id;
}

std::optional<Constants::Color> JoinGameRequestPacket::getPreferredColor() const
{
   return _preferred_color;
}

// the preferred color is an optional trailing field: without one the packet is the same as
// before, so servers that don't know the field are unaffected
void JoinGameRequestPacket::enqueue(BinaryWriter& out)
{
   out << _id;

   if (_preferred_color)
   {
      out << static_cast<int32_t>(*_preferred_color);
   }
}

void JoinGameRequestPacket::dequeue(BinaryReader& in)
{
   in >> _id;

   if (in.bytesAvailable() >= sizeof(int32_t))
   {
      int32_t color = 0;
      in >> color;
      if (color >= Constants::ColorWhite && color <= Constants::ColorOrange)
      {
         _preferred_color = static_cast<Constants::Color>(color);
      }
   }
}

void JoinGameRequestPacket::debug()
{
   qDebug("JoinGameRequestPacket:debug: id: %d, preferred color: %d", _id, _preferred_color ? static_cast<int>(*_preferred_color) : 0);
}
