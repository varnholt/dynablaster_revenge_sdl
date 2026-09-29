#include "errorpacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "Error";
}

ErrorPacket::ErrorPacket(Constants::ErrorType error_type, const std::string& message)
    : Packet(Packet::ERROR), _error_type(error_type), _error_message(message)
{
   _packet_name = PACKETNAME;
}

ErrorPacket::ErrorPacket() : Packet(Packet::ERROR)
{
   _packet_name = PACKETNAME;
}

void ErrorPacket::enqueue(BinaryWriter& out)
{
   out << static_cast<int32_t>(_error_type);
   out << _error_message;
}

void ErrorPacket::dequeue(BinaryReader& in)
{
   int32_t error_type = 0;
   in >> error_type;
   in >> _error_message;

   _error_type = static_cast<Constants::ErrorType>(error_type);
}

Constants::ErrorType ErrorPacket::getErrorType() const
{
   return _error_type;
}

void ErrorPacket::setErrorType(Constants::ErrorType error_type)
{
   _error_type = error_type;
}

const std::string& ErrorPacket::getErrorMessage() const
{
   return _error_message;
}

void ErrorPacket::setErrorMessage(const std::string& message)
{
   _error_message = message;
}

void ErrorPacket::debug()
{
   qDebug("ErrorPacket:debug: type: %d, message: %s", _error_type, _error_message.c_str());
}
