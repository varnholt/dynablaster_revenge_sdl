// header
#include "connection.h"

// shared
#include "packet.h"

Connection::Connection(int32_t id, NetStreamSocketHandle socket, int32_t player_id)
    : _id(id), _socket(std::move(socket)), _player(player_id)
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
   return NET_ReadFromStreamSocket(_socket.get(), buffer.data(), static_cast<int>(buffer.size()));
}

void Connection::write(const Packet& packet)
{
   NET_WriteToStreamSocket(_socket.get(), packet.data(), static_cast<int>(packet.size()));
}

std::string Connection::getPeerAddress() const
{
   const NetAddressHandle address(NET_GetStreamSocketAddress(_socket.get()), &NET_UnrefAddress);
   return address ? NET_GetAddressString(address.get()) : "?";
}
