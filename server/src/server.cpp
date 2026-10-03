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

Server::Server()
{
   initServerConfiguration();

   qDebug("Server::Server: Dynablaster Revenge Server - v%s, rev. %s", SERVER_VERSION, SERVER_REVISION);

   qDebug("Server::Server: binding to port %d..", SERVER_PORT);

   // create server, listening on all local addresses
   _net_server.reset(NET_CreateServer(nullptr, SERVER_PORT, 0));

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
   _connections.clear();
   _games.clear();
   _removed_games.clear();
}

bool Server::isListening() const
{
   return _net_server != nullptr;
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

std::optional<std::reference_wrapper<Game>> Server::findGame(const Connection& connection) const
{
   const auto mapping = _connection_games.find(connection.getId());

   if (mapping == _connection_games.end())
   {
      return std::nullopt;
   }

   const auto game_iterator = _games.find(mapping->second);

   if (game_iterator == _games.end())
   {
      return std::nullopt;
   }

   return *game_iterator->second;
}

void Server::acceptConnections()
{
   while (true)
   {
      // C API out parameter, owned by the connection right away
      NET_StreamSocket* accepted_socket = nullptr;

      if (!NET_AcceptClient(_net_server.get(), &accepted_socket))
      {
         qDebug("Server::acceptConnections: accept failed: %s", SDL_GetError());
         break;
      }

      if (!accepted_socket)
      {
         break;
      }

      const auto connection_id = _connection_id++;

      auto connection =
         std::make_unique<Connection>(connection_id, NetStreamSocketHandle(accepted_socket, &NET_DestroyStreamSocket), _player_id++);

      qDebug("Server::acceptConnections: peer address: %s, connection: %d", connection->getPeerAddress().c_str(), connection_id);

      _connections[connection_id] = std::move(connection);
   }
}

void Server::poll()
{
   acceptConnections();

   // snapshot the ids since disconnectSocket() mutates _connections mid-iteration
   std::vector<int32_t> connection_ids;
   connection_ids.reserve(_connections.size());
   for (const auto connection_id : _connections | std::views::keys)
   {
      connection_ids.push_back(connection_id);
   }

   for (const auto connection_id : connection_ids)
   {
      readSocket(connection_id);
   }
}

void Server::sendPacket(Connection& connection, std::unique_ptr<Packet> packet)
{
   packet->serialize();
   connection.write(*packet);
}

void Server::processStartGameRequest(Connection& connection, const StartGameRequestPacket& request)
{
   // TODO:
   // game needs "isRunning"
   // -> to be checked before restarting

   const auto game_iterator = _games.find(request.getId());

   if (game_iterator != _games.end())
   {
      Game& game = *game_iterator->second;

      if (connection.getPlayer().getId() == game.getCreatorId())
      {
         game.startSynchronization();
      }
   }
}

void Server::processJoinGameRequest(Connection& connection, const JoinGameRequestPacket& request)
{
   const auto game_iterator = _games.find(request.getId());

   if (game_iterator != _games.end())
   {
      Game& game = *game_iterator->second;

      if (game.joinGame(connection, request.getPreferredColor()))
      {
         game.broadcastMessage(std::format("{} joined the game", connection.getPlayer().getNick()));

         _connection_games[connection.getId()] = game.getId();

         // if the game is currently running, inform the player that the
         // game has been started
         game.processSpectator(connection);
      }
   }
}

void Server::processPlayerSynchronize(Connection& connection, const PlayerSynchronizePacket& request)
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

   if (request.getSynchronizeProcess() == PlayerSynchronizePacket::LevelLoaded)
   {
      if (const auto game = findGame(connection))
      {
         Player& player = connection.getPlayer();

         player.setLoadingSynchronized(true);

         qDebug("Server::processPlayerSynchronize: game: %d player: '%s'", game->get().getId(), player.getNick().c_str());
      }
   }
}

void Server::processLoginRequest(Connection& connection, const LoginRequestPacket& request)
{
   qDebug("Server::processLoginRequest: login request packet received");

   Player& player = connection.getPlayer();

   const bool was_logged_in = player.isLoggedIn();

   std::string nick = request.getNick();
   if (!was_logged_in)
   {
      nick = correctDuplicatePlayerName(nick);
   }

   // init player attributes
   player.setLoggedIn(true);
   player.setNick(nick);
   player.setBot(request.isBot());

   // send login acceptance directly back to sender
   sendPacket(connection, std::make_unique<LoginResponsePacket>(false, player.getId(), player.getNick(), getServerConfiguration()));
}

void Server::processListGamesRequest(Connection& connection)
{
   std::vector<GameInformation> games;

   for (const auto& game : _games | std::views::values)
   {
      games.push_back(game->getGameInformation());
   }

   sendPacket(connection, std::make_unique<ListGamesResponsePacket>(games));
}

void Server::processCreateGameRequest(Connection& connection, const CreateGameRequestPacket& request)
{
   // the server current does not support a maximum game count.
   // if this ought to be implemented, we need to return a gameinformation
   // object countaining a gameid of -1.

   auto game_owner = std::make_unique<Game>();
   Game& game = *game_owner;
   game.setCreateGameData(request.getData());
   game.setCreatorId(connection.getPlayer().getId());
   game.getGameRound().setCount(request.getData()._rounds);

   game.forceLeaveGameSignal.connect([this](Connection& leaving_connection) { processPlayerLeavesGame(leaving_connection); });

   // autocorrect duplicate game names
   correctDuplicateGameName(game);

   _games[game.getId()] = std::move(game_owner);

   game.initialize();

   sendPacket(connection, std::make_unique<CreateGameResponsePacket>(game.getGameInformation()));
}

void Server::processGamePacket(Connection& connection, const Packet& packet)
{
   // if the connection already joined a game, let the
   // according game instance handle the communication
   if (const auto game = findGame(connection))
   {
      game->get().processPacket(connection, packet);
   }
}

/*!
   data received from client

   concept:

      Server
      |
      + NET_Server
      + acceptConnections() => new Connection(new Player)
                                   |
                                   + NET_StreamSocket
      + readSocket() => while(data is available)
                  1) read from the connection's socket
                  2) deserialize Packet
                  3) process all server-related packets
                  4) process all game-related packets
*/
void Server::readSocket(int32_t connection_id)
{
   const auto connection_iterator = _connections.find(connection_id);

   if (connection_iterator == _connections.end())
   {
      return;
   }

   Connection& connection = *connection_iterator->second;
   PacketStreamBuffer& buffer = connection.getBuffer();

   std::array<char, 4096> chunk{};
   int bytes_read = 0;

   while ((bytes_read = connection.read(chunk)) > 0)
   {
      buffer.append(std::span(chunk).first(static_cast<size_t>(bytes_read)));
   }

   if (bytes_read < 0)
   {
      disconnectSocket(connection_id);
      return;
   }

   while (true)
   {
      uint16_t block_size = connection.getExpectedPacketSize();

      // blocksize not initialized yet
      if (block_size == 0)
      {
         if (buffer.bytesAvailable() < sizeof(uint16_t))
         {
            break;
         }

         BinaryReader size_reader = buffer.reader();
         size_reader >> block_size;
         buffer.consume(size_reader.pos());
         connection.setExpectedPacketSize(block_size);
      }

      // wait for more data
      if (buffer.bytesAvailable() < block_size)
      {
         break;
      }

      // reset expected blocksize
      connection.setExpectedPacketSize(0);

      // block was read completely; reading is limited to it, so a packet with trailing fields
      // this server doesn't know (or one shorter than expected) can't desync the stream
      BinaryReader in = buffer.reader(block_size);
      const auto packet = Packet::deserialize(in);
      buffer.consume(block_size);

      if (packet)
      {
         switch (packet->getType())
         {
            case Packet::CREATEGAMEREQUEST:
            {
               processCreateGameRequest(connection, static_cast<const CreateGameRequestPacket&>(*packet));
               break;
            }

            case Packet::LISTGAMESREQUEST:
            {
               processListGamesRequest(connection);
               break;
            }

            case Packet::LEAVEGAMEREQUEST:
            {
               processPlayerLeavesGame(connection);
               break;
            }

            case Packet::LOGINREQUEST:
            {
               processLoginRequest(connection, static_cast<const LoginRequestPacket&>(*packet));
               break;
            }

            case Packet::JOINGAMEREQUEST:
            {
               processJoinGameRequest(connection, static_cast<const JoinGameRequestPacket&>(*packet));
               break;
            }

            case Packet::STARTGAMEREQUEST:
            {
               processStartGameRequest(connection, static_cast<const StartGameRequestPacket&>(*packet));
               break;
            }

            case Packet::PLAYERMODIFIED:
            {
               break;
            }

            case Packet::PLAYERSYNCHRONIZEPACKET:
            {
               processPlayerSynchronize(connection, static_cast<const PlayerSynchronizePacket&>(*packet));
               break;
            }

            case Packet::INVALID:
            {
               qWarning("Server::readSocket: Packet::INVALID received");
               break;
            }

            default:
            {
               processGamePacket(connection, *packet);
               break;
            }
         }
      }
   }

   buffer.compact();
}

void Server::disconnectSocket(int32_t connection_id)
{
   qDebug("Server::disconnectSocket");

   const auto connection_iterator = _connections.find(connection_id);

   if (connection_iterator == _connections.end())
   {
      return;
   }

   // notify other players
   processPlayerLeavesGame(*connection_iterator->second);

   // destroys the player, the buffer and the socket
   _connections.erase(connection_iterator);
}

void Server::processBroadcastLeaveGameResponse(const Player& player, Game& game)
{
   for (Connection& connection : game.getConnections() | std::views::values)
   {
      qDebug(
         "Server::processBroadcastLeaveGameResponse: "
         "informing '%s' that '%s' left",
         connection.getPlayer().getNick().c_str(),
         player.getNick().c_str()
      );

      sendPacket(connection, std::make_unique<LeaveGameResponsePacket>(game.getId(), player.getId()));
   }
}

void Server::processPlayerLeavesGame(Connection& connection)
{
   const auto game = findGame(connection);

   // remove player from game
   if (!game)
   {
      _connection_games.erase(connection.getId());
      return;
   }

   const Player& player = connection.getPlayer();
   Game& left_game = *game;

   // notify all players in the game that player left
   processBroadcastLeaveGameResponse(player, left_game);

   // remove player from game
   left_game.removePlayer(connection);

   // remove connection from connection<->game-mapping
   _connection_games.erase(connection.getId());

   // if game is empty, delete game
   if (left_game.getPlayerCount() == 0 || left_game.getPlayerCount() == left_game.getBotCount())
   {
      const int game_id = left_game.getId();
      processRemoveAllBots(game_id);
      processRemoveGame(game_id);
   }
   else
   {
      left_game.broadcastMessage(std::format("{} left the game", player.getNick()));

      if (player.getId() == left_game.getCreatorId())
      {
         // 1) pass owner flag
         std::vector<std::reference_wrapper<Connection>> connections;
         for (Connection& remaining : left_game.getConnections() | std::views::values)
         {
            connections.push_back(remaining);
         }

         const auto new_owner =
            std::ranges::find_if(connections, [](const Connection& candidate) { return !candidate.getPlayer().isBot(); });

         if (new_owner != connections.end())
         {
            left_game.setCreatorId(new_owner->get().getPlayer().getId());
            left_game.broadcastMessage(std::format("{} is the new game owner", new_owner->get().getPlayer().getNick()));
         }

         // 2) notify players about new owner
         for (Connection& remaining : connections)
         {
            processListGamesRequest(remaining);
         }
      }

      // end game if only 1 player left
      if (left_game.getPlayerCount() == 1)
      {
         // it is not desired to have a game over condition while
         // the game is not even running :)
         if (left_game.getState() != Constants::GameStopped)
         {
            left_game.updateGameoverCondition();
         }
      }
   }
}

void Server::processRemoveGame(int game_id)
{
   const auto game_iterator = _games.find(game_id);

   if (game_iterator != _games.end())
   {
      // deferred: this may run from inside one of the game's own callbacks
      _removed_games.push_back(std::move(game_iterator->second));
      _games.erase(game_iterator);

      Timer::singleShot(0, [this]() { _removed_games.clear(); });
   }
}

void Server::processRemoveAllBots(int game_id)
{
   const auto game_iterator = _games.find(game_id);

   if (game_iterator != _games.end())
   {
      Game& game = *game_iterator->second;

      std::vector<std::reference_wrapper<Connection>> connections;
      for (Connection& connection : game.getConnections() | std::views::values)
      {
         connections.push_back(connection);
      }

      for (Connection& connection : connections)
      {
         // remove bot from game
         game.removePlayer(connection);

         // notify bot: "you're out"
         sendPacket(connection, std::make_unique<LeaveGameResponsePacket>(game_id, connection.getPlayer().getId()));

         // remove connection from connection<->game-mapping
         _connection_games.erase(connection.getId());
      }
   }
}

void Server::correctDuplicateGameName(Game& new_game)
{
   const std::string game_name = new_game.getName();
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
      new_game.setName(corrected_game_name);
   }
}

std::string Server::correctDuplicatePlayerName(const std::string& nick)
{
   std::string corrected_nick = nick;

   const auto is_duplicate = [this, &corrected_nick]()
   {
      return std::ranges::any_of(
         _connections | std::views::values,
         [&corrected_nick](const auto& connection) { return corrected_nick == connection->getPlayer().getNick(); }
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
