#include "loginrequestpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "LoginRequest";
}

LoginRequestPacket::LoginRequestPacket(const std::string& nick, bool bot) : Packet(Packet::LOGINREQUEST), _nick(nick), _bot(bot)
{
   _packet_name = PACKETNAME;
}

LoginRequestPacket::LoginRequestPacket() : Packet(Packet::LOGINREQUEST)
{
   _packet_name = PACKETNAME;
}

const std::string& LoginRequestPacket::getNick() const
{
   return _nick;
}

bool LoginRequestPacket::isBot() const
{
   return _bot;
}

void LoginRequestPacket::enqueue(BinaryWriter& out)
{
   out << _nick;
   out << _bot;
}

void LoginRequestPacket::dequeue(BinaryReader& in)
{
   in >> _nick;
   in >> _bot;
}

void LoginRequestPacket::debug()
{
   qDebug("LoginRequestPacket:loginrequest: player: %s, bot: %d", _nick.c_str(), _bot);
}
