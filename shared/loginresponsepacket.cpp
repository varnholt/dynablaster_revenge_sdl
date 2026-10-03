#include "loginresponsepacket.h"

#include "logging.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "LoginResponse";
}

LoginResponsePacket::LoginResponsePacket(
   bool broadcast,
   int32_t id,
   const std::string& nick,
   const ServerConfiguration& server_configuration
)
    : Packet(Packet::LOGINRESPONSE), _broadcast(broadcast), _id(id), _nick(nick), _server_configuration(server_configuration)
{
   _packet_name = PACKETNAME;
}

LoginResponsePacket::LoginResponsePacket() : Packet(Packet::LOGINRESPONSE)
{
   _packet_name = PACKETNAME;
}

void LoginResponsePacket::setId(int32_t id)
{
   _id = id;
}

int32_t LoginResponsePacket::getId() const
{
   return _id;
}

void LoginResponsePacket::enqueue(BinaryWriter& out)
{
   out << _broadcast << _id << _nick << _server_configuration;
}

void LoginResponsePacket::dequeue(BinaryReader& in)
{
   in >> _broadcast >> _id >> _nick >> _server_configuration;
}

void LoginResponsePacket::debug()
{
   qDebug("LoginResponsePacket:loginresponse: player: %d (%s)", _id, (_id != -1) ? "accepted" : "denied");
}

const std::string& LoginResponsePacket::getNick() const
{
   return _nick;
}

bool LoginResponsePacket::isBroadcast() const
{
   return _broadcast;
}

const ServerConfiguration& LoginResponsePacket::getServerConfiguration() const
{
   return _server_configuration;
}
