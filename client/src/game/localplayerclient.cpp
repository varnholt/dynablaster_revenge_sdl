#include "localplayerclient.h"

#include "constants.h"
#include "gameeventpacket.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "keypacket.h"
#include "leavegamerequestpacket.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "packet.h"
#include "playerkilledpacket.h"
#include "playersynchronizepacket.h"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include <array>

namespace
{
constexpr uint16_t server_port = 6300;
constexpr int32_t poll_interval_ms = 16;
}  // namespace

LocalPlayerClient::LocalPlayerClient(std::string host, std::string nick, int32_t game_id)
    : _host(std::move(host)), _nick(std::move(nick)), _game_id(game_id)
{
   _poll_timer.timeoutSignal.connect([this]() { poll(); });
   _poll_timer.start(poll_interval_ms);

   _address = NET_ResolveHostname(_host.c_str());
   if (!_address)
   {
      SDL_Log("LocalPlayerClient: cannot resolve %s", _host.c_str());
   }
}

LocalPlayerClient::~LocalPlayerClient()
{
   leave();
}

void LocalPlayerClient::poll()
{
   if (_address)
   {
      const NET_Status status = NET_GetAddressStatus(_address);
      if (status == NET_SUCCESS)
      {
         _socket = NET_CreateClient(_address, server_port, 0);
         NET_UnrefAddress(_address);
         _address = nullptr;
      }
      else if (status == NET_FAILURE)
      {
         NET_UnrefAddress(_address);
         _address = nullptr;
         joinFailedSignal();
      }
      return;
   }

   if (_socket && !_connected)
   {
      const NET_Status status = NET_GetConnectionStatus(_socket);
      if (status == NET_SUCCESS)
      {
         _connected = true;
         LoginRequestPacket login(_nick, false);
         send(&login);
      }
      else if (status == NET_FAILURE)
      {
         disconnect();
         joinFailedSignal();
      }
      return;
   }

   if (_socket)
   {
      readData();
   }
}

void LocalPlayerClient::readData()
{
   std::array<char, 4096> chunk{};
   int32_t bytes_read = 0;
   while ((bytes_read = NET_ReadFromStreamSocket(_socket, chunk.data(), static_cast<int>(chunk.size()))) > 0)
   {
      _buffer.append(chunk.data(), static_cast<size_t>(bytes_read));
   }

   if (bytes_read < 0)
   {
      disconnect();
      return;
   }

   while (packetAvailable())
   {
      BinaryReader in = _buffer.reader();
      auto packet = Packet::deserialize(in);
      _buffer.consume(in.pos());
      _block_size = 0;

      if (packet)
      {
         processPacket(packet.get());
      }
   }

   _buffer.compact();
}

bool LocalPlayerClient::packetAvailable()
{
   if (_block_size == 0)
   {
      if (_buffer.bytesAvailable() < sizeof(uint16_t))
      {
         return false;
      }

      BinaryReader size_reader = _buffer.reader();
      size_reader >> _block_size;
      _buffer.consume(size_reader.pos());
   }

   return _buffer.bytesAvailable() >= _block_size;
}

void LocalPlayerClient::processPacket(Packet* packet)
{
   switch (packet->getType())
   {
      case Packet::LOGINRESPONSE:
      {
         auto* response = static_cast<LoginResponsePacket*>(packet);
         _player_id = response->getId();
         if (_player_id < 0)
         {
            joinFailedSignal();
            break;
         }

         JoinGameRequestPacket join(_game_id);
         send(&join);
         break;
      }

      case Packet::JOINGAMERESPONSE:
      {
         // join responses are broadcast for every player, only ours matters
         auto* response = static_cast<JoinGameResponsePacket*>(packet);
         if (response->getPlayerId() != _player_id || _joined)
         {
            break;
         }

         if (!response->isSuccessful())
         {
            joinFailedSignal();
            break;
         }

         // nothing to load, ready right away
         _joined = true;
         PlayerSynchronizePacket sync(PlayerSynchronizePacket::LevelLoaded);
         send(&sync);
         joinedSignal(_player_id);
         break;
      }

      case Packet::PLAYERKILLED:
      {
         if (static_cast<PlayerKilledPacket*>(packet)->getPlayerId() == _player_id)
         {
            rumbleSignal(0.5f, 2000);
         }
         break;
      }

      case Packet::GAMEEVENT:
      {
         auto* event = static_cast<GameEventPacket*>(packet);
         if (event->getGameEvent() == GameEventPacket::ExtraCollected && event->getPlayerId() == _player_id)
         {
            rumbleSignal(0.2f, 500);
         }
         break;
      }

      default:
         break;
   }
}

void LocalPlayerClient::send(Packet* packet)
{
   if (!_socket || !_connected)
   {
      return;
   }

   packet->serialize();
   NET_WriteToStreamSocket(_socket, packet->constData(), static_cast<int>(packet->size()));
}

void LocalPlayerClient::setKeys(uint8_t keys)
{
   // like BombermanClient, a held bomb key drops one bomb only
   if (!(keys & Constants::KeyBomb))
   {
      _bomb_released = true;
   }
   else if (!_bomb_released)
   {
      keys &= ~Constants::KeyBomb;
   }

   if (keys == _keys)
   {
      return;
   }

   _keys = keys;
   sendKeys();

   if (_keys & Constants::KeyBomb)
   {
      _bomb_released = false;
      _keys &= ~Constants::KeyBomb;
   }
}

void LocalPlayerClient::sendKeys()
{
   if (_joined)
   {
      KeyPacket packet(0, static_cast<int8_t>(_keys));
      send(&packet);
   }
}

void LocalPlayerClient::leave()
{
   if (_joined)
   {
      LeaveGameRequestPacket packet(_game_id, _player_id);
      send(&packet);
      _joined = false;
   }

   disconnect();
}

void LocalPlayerClient::disconnect()
{
   _poll_timer.stop();

   if (_address)
   {
      NET_UnrefAddress(_address);
      _address = nullptr;
   }

   if (_socket)
   {
      NET_WaitUntilStreamSocketDrained(_socket, 100);
      NET_DestroyStreamSocket(_socket);
      _socket = nullptr;
   }

   _connected = false;
   _joined = false;
}

int32_t LocalPlayerClient::getPlayerId() const
{
   return _player_id;
}

bool LocalPlayerClient::isJoined() const
{
   return _joined;
}

const std::string& LocalPlayerClient::getNick() const
{
   return _nick;
}
