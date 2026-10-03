#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// server
#include "connection.h"

// shared
#include "constants.h"
#include "nethandles.h"
#include "packet.h"
#include "serverconfiguration.h"
#include "timer.h"

// forward declarations
class CreateGameRequestPacket;
class Game;
class JoinGameRequestPacket;
class LoginRequestPacket;
class Player;
class PlayerSynchronizePacket;
class StartGameRequestPacket;

class Server
{
public:
   Server();
   ~Server();

   //! check if server is listening
   bool isListening() const;

   //! getter for server configuration
   const ServerConfiguration& getServerConfiguration() const;

   //! start the poll timer
   void startPolling();

protected:
   //! initialize server configuration
   void initServerConfiguration();

private:
   //! poll for new connections and incoming data, once per tick
   void poll();

   void processStartGameRequest(Connection& connection, const StartGameRequestPacket& request);
   void processJoinGameRequest(Connection& connection, const JoinGameRequestPacket& request);
   void processLoginRequest(Connection& connection, const LoginRequestPacket& request);
   void processListGamesRequest(Connection& connection);
   void processCreateGameRequest(Connection& connection, const CreateGameRequestPacket& request);
   void processGamePacket(Connection& connection, const Packet& packet);
   void processPlayerLeavesGame(Connection& connection);
   void processPlayerSynchronize(Connection& connection, const PlayerSynchronizePacket& request);
   void processRemoveGame(int game_id);
   void processRemoveAllBots(int game_id);
   void processBroadcastLeaveGameResponse(const Player& player, Game& game);

   //! fix duplicate game names
   void correctDuplicateGameName(Game& new_game);

   //! send single packet
   void sendPacket(Connection& connection, std::unique_ptr<Packet> packet);

   //! accept all pending incoming connections
   void acceptConnections();

   //! read and dispatch all available data for one connection
   void readSocket(int32_t connection_id);

   //! socket failed or the remote end dropped - clean up and destroy the connection
   void disconnectSocket(int32_t connection_id);

   //! fix duplicate player names
   std::string correctDuplicatePlayerName(const std::string& nick);

   //! game the connection joined, if any
   std::optional<std::reference_wrapper<Game>> findGame(const Connection& connection) const;

   //! listen socket
   NetServerHandle _net_server{nullptr, &NET_DestroyServer};

   //! drives poll() once per tick
   Timer _poll_timer;

   //! map connection id <-> connection, owns the connections and their players
   std::map<int32_t, std::unique_ptr<Connection>> _connections;

   //! map connection id <-> id of the game the connection joined
   std::map<int32_t, int> _connection_games;

   //! map of active games, owns the games
   std::map<int, std::unique_ptr<Game>> _games;

   //! removed games, destroyed on the next tick
   std::vector<std::unique_ptr<Game>> _removed_games;

   //! next player id
   int _player_id = 0;

   //! next connection id
   int32_t _connection_id = 0;

   //! server configuration data
   ServerConfiguration _server_configuration;
};

#endif
