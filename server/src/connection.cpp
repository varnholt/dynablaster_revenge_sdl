// header
#include "connection.h"

// shared
#include "packet.h"

Connection::Connection(int32_t id, std::unique_ptr<ByteStream> stream, int32_t player_id)
    : _id(id), _stream(std::move(stream)), _player(player_id)
{
}

int32_t Connection::getId() const
{
   return _id;
}

Player& Connection::getPlayer()
{
   return _player;
}

const Player& Connection::getPlayer() const
{
   return _player;
}

PacketStreamBuffer& Connection::getBuffer()
{
   return _buffer;
}

uint16_t Connection::getExpectedPacketSize() const
{
   return _expected_packet_size;
}

void Connection::setExpectedPacketSize(uint16_t size)
{
   _expected_packet_size = size;
}

int32_t Connection::read(std::span<char> buffer)
{
   return _stream->read(buffer);
}

void Connection::write(const Packet& packet)
{
   _stream->write(packet);
}

std::string Connection::getPeerAddress() const
{
   return _stream->getPeerAddress();
}
