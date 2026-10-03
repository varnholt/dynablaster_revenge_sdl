#ifndef CONNECTION_H
#define CONNECTION_H

#include <cstdint>
#include <span>
#include <string>

// shared
#include "nethandles.h"
#include "packetstreambuffer.h"
#include "player.h"

class Packet;

//! one client connected to the server: its socket, its incoming bytes and its player
class Connection
{
public:
   Connection(int32_t id, NetStreamSocketHandle socket, int32_t player_id);

   Connection(const Connection&) = delete;
   Connection& operator=(const Connection&) = delete;

   //! unique id of this connection
   int32_t getId() const;

   //! the connected player
   Player& getPlayer();
   const Player& getPlayer() const;

   //! bytes received but not processed yet
   PacketStreamBuffer& getBuffer();

   //! size of the packet currently being received, 0 if unknown yet
   uint16_t getExpectedPacketSize() const;
   void setExpectedPacketSize(uint16_t size);

   //! read what is available, returns the number of bytes read or a negative value on failure
   int32_t read(std::span<char> buffer);

   //! write a serialized packet
   void write(const Packet& packet);

   //! address of the peer, for logging
   std::string getPeerAddress() const;

private:
   int32_t _id = 0;
   NetStreamSocketHandle _socket;
   Player _player;
   PacketStreamBuffer _buffer;
   uint16_t _expected_packet_size = 0;
};

#endif  // CONNECTION_H
