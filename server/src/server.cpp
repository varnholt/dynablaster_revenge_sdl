// header
#include "server.h"

// server
#include "game.h"
#include "serverversion.h"

// shared
#include "creategamerequestpacket.h"
#include "creategameresponsepacket.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "leavegamerequestpacket.h"
#include "leavegameresponsepacket.h"
#include "listgamesrequestpacket.h"
#include "listgamesresponsepacket.h"
#include "logging.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "player.h"
#include "playersynchronizepacket.h"
#include "settings.h"
#include "startgamerequestpacket.h"
#include "startgameresponsepacket.h"
#include "stopgamerequestpacket.h"
#include "stopgameresponsepacket.h"

// stdlib
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <ranges>
#include <string>
#include <vector>

// SDL
#include <SDL3_net/SDL_net.h>

Server* Server::_instance = nullptr;

Server::Server()
{
   _instance = this;

   initServerConfiguration();

   qDebug("Server::Server: Dynablaster Revenge Server - v%s, rev. %s", SERVER_VERSION, SERVER_REVISION);

   qDebug("Server::Server: binding to port %d..", SERVER_PORT);

   // create server, listening on all local addresses
   _net_server = NET_CreateServer(nullptr, SERVER_PORT, 0);

   if (!_net_server)
   {
      qDebug("Server::Server: Dynablaster Revenge Server: Unable to start the server: %s.", SDL_GetError());
   }
   else
   {
      qDebug("Server::Server: Dynablaster Revenge Server: server initialized");
   }

   // check for incoming connections and incoming data once per tick - not started here,
   // see startPolling()
   _poll_timer.timeoutSignal.connect([this]() { poll(); });
}

//! start the poll timer - call once this object is running on its final thread
void Server::startPolling()
{
   _poll_timer.start(16);
}

Server::~Server()
{
   qDebug("Server::~Server");

   // players first: an infected player's disease disconnects itself from its game on destruction
   _player_sockets.clear();
   _games.clear();

   if (_net_server)
   {
      NET_DestroyServer(_net_server);
   }

   if (_instance == this)
   {
      _instance = nullptr;
   }
}

Server* Server::getInstance()
{
   if (!_instance)
   {
      new Server();
   }

   return _instance;
}

bool Server::isListening() const
{
   return _net_server != nullptr;
}

NET_StreamSocket* Server::getPlayerSocket(int player_id)
{
   const auto socket_iterator =
      std::ranges::find_if(_player_sockets, [player_id](const auto& entry) { return entry.second && entry.second->getId() == player_id; });

   return socket_iterator != _player_sockets.end() ? socket_iterator->first : nullptr;
}

const ServerConfiguration& Server::getServerConfiguration() const
{
   return _server_configuration;
}

void Server::initServerConfiguration()
{
   Settings settings(SERVER_CONFIG_FILE_SERVER, Settings::IniFormat);

   const int bomb_tick_time = settings.value("tick_count", SERVER_BOMB_TICKTIME_DEFAULT).toInt();

   _server_configuration.setBombTickTime(bomb_tick_time);
}

Player* Server::findPlayer(NET_StreamSocket* socket) const
{
   const auto player_iterator = _player_sockets.find(socket);
   return player_iterator != _player_sockets.end() ? player_iterator->second.get() : nullptr;
}

void Server::acceptConnections()
{
   while (true)
   {
      NET_StreamSocket* socket = nullptr;

      if (!NET_AcceptClient(_net_server, &socket))
      {
         qDebug("Server::acceptConnections: accept failed: %s", SDL_GetError());
         break;
      }

      if (!socket)
      {
         break;
      }

      NET_Address* address = NET_GetStreamSocketAddress(socket);

      qDebug(
         "Server::acceptConnections: peer address: %s, socket ptr: %p",
         address ? NET_GetAddressString(address) : "?",
         static_cast<void*>(socket)
      );

      if (address)
      {
         NET_UnrefAddress(address);
      }

      _player_sockets[socket] = std::make_unique<Player>(_player_id++);
   }
}

void Server::poll()
{
   acceptConnections();

   // snapshot the keys since disconnectSocket() mutates _player_sockets mid-iteration
   std::vector<NET_StreamSocket*> sockets;
   sockets.reserve(_player_sockets.size());
   for (const auto& [socket, player] : _player_sockets)
   {
      sockets.push_back(socket);
   }

   for (NET_StreamSocket* socket : sockets)
   {
      readSocket(socket);
   }
}

void Server::sendPacket(NET_StreamSocket* socket, std::unique_ptr<Packet> packet)
{
   packet->serialize();

   if (socket)
   {
      NET_WriteToStreamSocket(socket, packet->constData(), static_cast<int>(packet->size()));
   }
}

void Server::processStartGameRequest(NET_StreamSocket* tcp_socket, Packet* packet)
{
   auto* request = dynamic_cast<StartGameRequestPacket*>(packet);

   // TODO:
   // game needs "isRunning"
   // -> to be checked before restarting

   const auto game_iterator = _games.find(request->getId());

   if (game_iterator != _games.end())
   {
      Game* game = game_iterator->second.get();
      Player* player = findPlayer(tcp_socket);

      if (player && player->getId() == game->getCreator()->getId())
      {
         game->startSynchronization();
      }
   }
}

void Server::processJoinGameRequest(NET_StreamSocket* tcp_socket, Packet* packet)
{
   auto* request = dynamic_cast<JoinGameRequestPacket*>(packet);

   const auto game_iterator = _games.find(request->getId());

   if (game_iterator != _games.end())
   {
      Game* game = game_iterator->second.get();
      Player* player = findPlayer(tcp_socket);

      if (player)
      {
         if (game->joinGame(player, tcp_socket))
         {
            game->broadcastMessage(std::format("{} joined the game", player->getNick()));

            _socket_game_mapping[tcp_socket] = game;

            // if the game is currently running, inform the player that the
            // game has been started
            game->processSpectator(tcp_socket);
         }
      }
      else
      {
         sendPacket(tcp_socket, std::make_unique<JoinGameResponsePacket>(false, game->getId(), -1, "fuck off", Constants::ColorWhite));
      }
   }
}

void Server::processPlayerSynchronize(NET_StreamSocket* tcp_socket, Packet* packet)
{
   /*
      game level loading synchronizing workflow:

      1) player joins game
      2) client loads level data (in the background; via thread)
      3) client starts game
      4) server waits until all players will have sent a synch packet
      5) client finishes loading level data and sends synch-packet
      6) server delays until timeout (= 10 secs) or until all players sent a sync packet
      7) kill those players who didn't send a synch packet
      8) start game
   */

   auto* request = dynamic_cast<PlayerSynchronizePacket*>(packet);

   if (request->getSynchronizeProcess() == PlayerSynchronizePacket::LevelLoaded)
   {
      const auto game_iterator = _socket_game_mapping.find(tcp_socket);

      if (game_iterator != _socket_game_mapping.end())
      {
         Game* game = game_iterator->second;
         Player* player = findPlayer(tcp_socket);

         if (player)
         {
            player->setLoadingSynchronized(true);

            qDebug("Server::processPlayerSynchronize: game: %d player: '%s'", game->getId(), player->getNick().c_str());
         }
      }
   }
}

void Server::processLoginRequest(NET_StreamSocket* tcp_socket, Packet* packet)
{
   auto* request = dynamic_cast<LoginRequestPacket*>(packet);

   qDebug("Server::processLoginRequest: login request packet received");

   Player* player = findPlayer(tcp_socket);

   if (player)
   {
      const bool was_logged_in = player->isLoggedIn();

      std::string nick = request->getNick();
      if (!was_logged_in)
      {
         nick = correctDuplicatePlayerName(nick);
      }

      // init player attributes
      player->setLoggedIn(true);
      player->setNick(nick);
      player->setBot(request->isBot());

      // send login acceptance directly back to sender
      sendPacket(tcp_socket, std::make_unique<LoginResponsePacket>(false, player->getId(), player->getNick(), getServerConfiguration()));
   }
}

void Server::processListGamesRequest(NET_StreamSocket* tcp_socket)
{
   std::vector<GameInformation> games;

   for (const auto& game : _games | std::views::values)
   {
      games.push_back(game->getGameInformation());
   }

   sendPacket(tcp_socket, std::make_unique<ListGamesResponsePacket>(games));
}

void Server::processCreateGameRequest(NET_StreamSocket* tcp_socket, Packet* packet)
{
   // the server current does not support a maximum game count.
   // if this ought to be implemented, we need to return a gameinformation
   // object countaining a gameid of -1.

   auto* request = dynamic_cast<CreateGameRequestPacket*>(packet);

   auto game_owner = std::make_unique<Game>();
   Game* game = game_owner.get();
   game->setCreateGameData(request->getData());
   game->setCreator(findPlayer(tcp_socket));
   game->getGameRound()->setCount(request->getData()._rounds);

   game->forceLeaveGameSignal.connect([this](NET_StreamSocket* socket) { processPlayerLeavesGame(socket); });

   // autocorrect duplicate game names
   correctDuplicateGameName(game);

   _games[game->getId()] = std::move(game_owner);

   game->initialize();

   sendPacket(tcp_socket, std::make_unique<CreateGameResponsePacket>(game->getGameInformation()));
}

void Server::processGamePacket(NET_StreamSocket* tcp_socket, Packet* packet)
{
   const auto game_iterator = _socket_game_mapping.find(tcp_socket);

   // if the socket already joined a game, let the
   // according game instance handle the communication
   if (game_iterator != _socket_game_mapping.end())
   {
      game_iterator->second->processPacket(tcp_socket, packet);
   }
}

/*!
   data received from client

   concept:

      Server
      |
      + NET_Server*
      + acceptConnections() => new Player()
                                   |
                                   + NET_StreamSocket*
      + readSocket() => while(data is available)
                  1) read from NET_StreamSocket*
                  2) deserialize Packet*
                  3) process all server-related packets
                  4) process all game-related packets
*/
void Server::readSocket(NET_StreamSocket* tcp_socket)
{
   auto& buffer = _socket_buffers[tcp_socket];

   if (!buffer)
   {
      buffer = std::make_unique<PacketStreamBuffer>();
   }

   std::array<char, 4096> chunk{};
   int bytes_read = 0;

   while ((bytes_read = NET_ReadFromStreamSocket(tcp_socket, chunk.data(), static_cast<int>(chunk.size()))) > 0)
   {
      buffer->append(chunk.data(), static_cast<size_t>(bytes_read));
   }

   if (bytes_read < 0)
   {
      disconnectSocket(tcp_socket);
      return;
   }

   while (true)
   {
      uint16_t block_size = _packet_sizes[tcp_socket];

      // blocksize not initialized yet
      if (block_size == 0)
      {
         if (buffer->bytesAvailable() < sizeof(uint16_t))
         {
            break;
         }

         BinaryReader size_reader = buffer->reader();
         size_reader >> block_size;
         buffer->consume(size_reader.pos());
         _packet_sizes[tcp_socket] = block_size;
      }

      // wait for more data
      if (buffer->bytesAvailable() < block_size)
      {
         break;
      }

      // reset expected blocksize
      _packet_sizes[tcp_socket] = 0;

      // block was read completely
      BinaryReader in = buffer->reader();
      auto packet = Packet::deserialize(in);
      buffer->consume(in.pos());

      if (packet)
      {
         switch (packet->getType())
         {
            case Packet::CREATEGAMEREQUEST:
            {
               processCreateGameRequest(tcp_socket, packet.get());
               break;
            }

            case Packet::LISTGAMESREQUEST:
            {
               processListGamesRequest(tcp_socket);
               break;
            }

            case Packet::LEAVEGAMEREQUEST:
            {
               processPlayerLeavesGame(tcp_socket);
               break;
            }

            case Packet::LOGINREQUEST:
            {
               processLoginRequest(tcp_socket, packet.get());
               break;
            }

            case Packet::JOINGAMEREQUEST:
            {
               processJoinGameRequest(tcp_socket, packet.get());
               break;
            }

            case Packet::STARTGAMEREQUEST:
            {
               processStartGameRequest(tcp_socket, packet.get());
               break;
            }

            case Packet::PLAYERMODIFIED:
            {
               break;
            }

            case Packet::PLAYERSYNCHRONIZEPACKET:
            {
               processPlayerSynchronize(tcp_socket, packet.get());
               break;
            }

            case Packet::INVALID:
            {
               qWarning("Server::readSocket: Packet::INVALID received");
               break;
            }

            default:
            {
               processGamePacket(tcp_socket, packet.get());
               break;
            }
         }
      }
   }

   buffer->compact();
}

void Server::disconnectSocket(NET_StreamSocket* tcp_socket)
{
   qDebug("Server::disconnectSocket");

   // notify other players
   processPlayerLeavesGame(tcp_socket);

   // destroys the player and the socket's buffer
   _player_sockets.erase(tcp_socket);
   _socket_buffers.erase(tcp_socket);
   _packet_sizes.erase(tcp_socket);

   NET_DestroyStreamSocket(tcp_socket);
}

void Server::processBroadcastLeaveGameResponse(Player* player, Game* game)
{
   for (const auto& [socket, socket_player] : game->getPlayerSockets())
   {
      qDebug(
         "Server::processBroadcastLeaveGameResponse: "
         "informing '%s' that '%s' left",
         socket_player->getNick().c_str(),
         player->getNick().c_str()
      );

      sendPacket(socket, std::make_unique<LeaveGameResponsePacket>(game->getId(), player->getId()));
   }
}

void Server::processPlayerLeavesGame(NET_StreamSocket* tcp_socket)
{
   const auto game_iterator = _socket_game_mapping.find(tcp_socket);

   // remove player from game
   if (game_iterator != _socket_game_mapping.end())
   {
      Player* player = findPlayer(tcp_socket);
      Game* game = game_iterator->second;

      // notify all players in the game that player left
      processBroadcastLeaveGameResponse(player, game);

      // remove player from game
      game->removePlayer(player, tcp_socket);

      // remove socket from socket<->game-mapping
      _socket_game_mapping.erase(tcp_socket);

      // if game is empty, delete game
      if (game->getPlayerCount() == 0 || game->getPlayerCount() == game->getBotCount())
      {
         processRemoveAllBots(game->getId());
         processRemoveGame(game->getId());
      }
      else
      {
         game->broadcastMessage(std::format("{} left the game", player->getNick()));

         if (player == game->getCreator())
         {
            // 1) pass owner flag
            const std::vector<Player*> players = game->getPlayers();

            const auto new_owner = std::ranges::find_if(players, [](Player* candidate) { return !candidate->isBot(); });

            if (new_owner != players.end())
            {
               game->setCreator(*new_owner);
               game->broadcastMessage(std::format("{} is the new game owner", (*new_owner)->getNick()));
            }

            // 2) notify players about new owner
            for (Player* game_player : players)
            {
               processListGamesRequest(game->getSocket(game_player));
            }
         }

         // end game if only 1 player left
         if (game->getPlayerCount() == 1)
         {
            // it is not desired to have a game over condition while
            // the game is not even running :)
            if (game->getState() != Constants::GameStopped)
            {
               game->updateGameoverCondition();
            }
         }
      }
   }
}

void Server::processRemoveGame(int game_id)
{
   const auto game_iterator = _games.find(game_id);

   if (game_iterator != _games.end())
   {
      Game* game = game_iterator->second.release();
      _games.erase(game_iterator);

      // deferred: this may run from inside one of the game's own callbacks
      Timer::singleShot(0, [game]() { delete game; });
   }
}

void Server::processRemoveAllBots(int game_id)
{
   const auto game_iterator = _games.find(game_id);

   if (game_iterator != _games.end())
   {
      Game* game = game_iterator->second.get();

      const std::vector<Player*> players = game->getPlayers();
      for (Player* player : players)
      {
         const auto socket_iterator =
            std::ranges::find_if(_player_sockets, [player](const auto& entry) { return entry.second.get() == player; });

         NET_StreamSocket* tcp_socket = socket_iterator != _player_sockets.end() ? socket_iterator->first : nullptr;

         // remove bot from game
         game->removePlayer(player, tcp_socket);

         if (tcp_socket)
         {
            // notify bot: "you're out"
            sendPacket(tcp_socket, std::make_unique<LeaveGameResponsePacket>(game_id, player->getId()));

            // remove socket from socket<->game-mapping
            _socket_game_mapping.erase(tcp_socket);
         }
      }
   }
}

void Server::correctDuplicateGameName(Game* new_game)
{
   const std::string game_name = new_game->getName();
   std::string corrected_game_name = game_name;

   const auto is_duplicate = [this, &corrected_game_name]()
   {
      return std::ranges::any_of(
         _games | std::views::values, [&corrected_game_name](const auto& game) { return game->getName() == corrected_game_name; }
      );
   };

   int iteration = 0;
   bool changed = false;

   while (is_duplicate())
   {
      changed = true;
      corrected_game_name = std::format("{} #{}", game_name, iteration + 1);
      iteration++;
   }

   if (changed)
   {
      new_game->setName(corrected_game_name);
   }
}

std::string Server::correctDuplicatePlayerName(const std::string& nick)
{
   std::string corrected_nick = nick;

   const auto is_duplicate = [this, &corrected_nick]()
   {
      return std::ranges::any_of(
         _player_sockets | std::views::values, [&corrected_nick](const auto& player) { return corrected_nick == player->getNick(); }
      );
   };

   int iteration = 0;

   while (is_duplicate())
   {
      corrected_nick = std::format("{}{}", nick, iteration + 1);
      iteration++;
   }

   return corrected_nick;
}
