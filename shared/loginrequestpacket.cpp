#include "loginrequestpacket.h"

#include "log.h"

#include <format>
#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "LoginRequest";
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
   Log::info(std::format("LoginRequestPacket:loginrequest: player: {}, bot: {}", _nick, _bot));
}
