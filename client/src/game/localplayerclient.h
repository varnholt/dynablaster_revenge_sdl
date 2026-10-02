#pragma once

#include "gamesignal.h"
#include "packetstreambuffer.h"
#include "timer.h"

#include <cstdint>
#include <string>

class Packet;
struct NET_Address;
struct NET_StreamSocket;

/// \brief an additional player on this machine. the server knows one player per connection, so
/// every local player beyond the first gets its own: it logs in, joins the main client's game
/// and only sends key states. everything shown on screen comes from the main BombermanClient.
class LocalPlayerClient
{
public:
   LocalPlayerClient(std::string host, std::string nick, int32_t game_id);
   LocalPlayerClient(const LocalPlayerClient&) = delete;
   LocalPlayerClient& operator=(const LocalPlayerClient&) = delete;
   ~LocalPlayerClient();

   /// \brief Constants::Key mask of what this player currently holds
   void setKeys(uint8_t keys);

   /// \brief leaves the game and closes the connection
   void leave();

   int32_t getPlayerId() const;
   bool isJoined() const;
   const std::string& getNick() const;

   Signal<int32_t> joinedSignal;
   Signal<> joinFailedSignal;
   Signal<float, int32_t> rumbleSignal;

private:
   void poll();
   void readData();
   bool packetAvailable();
   void processPacket(Packet* packet);
   void send(Packet* packet);
   void sendKeys();
   void disconnect();

   std::string _host;
   std::string _nick;
   int32_t _game_id = -1;
   int32_t _player_id = -1;
   bool _joined = false;
   bool _connected = false;

   uint8_t _keys = 0;
   bool _bomb_released = true;

   NET_Address* _address = nullptr;
   NET_StreamSocket* _socket = nullptr;
   PacketStreamBuffer _buffer;
   uint16_t _block_size = 0;
   Timer _poll_timer;
};
