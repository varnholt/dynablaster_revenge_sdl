#include "messagepacket.h"

#include "logging.h"

namespace
{
constexpr auto PACKETNAME = "Message";
}

MessagePacket::MessagePacket(int8_t sender_id, const std::string& message, bool finished_typing, int8_t receiver_id)
    : Packet(Packet::MESSAGE), _sender_id(sender_id), _message(message), _receiver_id(receiver_id), _finished_typing(finished_typing)
{
   _packet_name = PACKETNAME;
}

MessagePacket::MessagePacket() : Packet(Packet::MESSAGE)
{
   _packet_name = PACKETNAME;
}

std::string MessagePacket::getMessage() const
{
   return _message;
}

int8_t MessagePacket::getReceiverId() const
{
   return _receiver_id;
}

bool MessagePacket::isTypingFinished() const
{
   return _finished_typing;
}

void MessagePacket::enqueue(BinaryWriter& out)
{
   out << _sender_id;
   out << _message;
   out << _receiver_id;
   out << _finished_typing;
}

void MessagePacket::dequeue(BinaryReader& in)
{
   in >> _sender_id >> _message >> _receiver_id >> _finished_typing;
}

int8_t MessagePacket::getSenderId() const
{
   return _sender_id;
}

void MessagePacket::debug()
{
   qDebug(
      "MessagePacket:debug: sender: %d, message: '%s', "
      "receiver: %d, finished: %d",
      _sender_id,
      _message.c_str(),
      _receiver_id,
      _finished_typing
   );
}
