#ifndef SERVER_H
#define SERVER_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>

// shared
#include "constants.h"
#include "packet.h"
#include "packetstreambuffer.h"
#include "serverconfiguration.h"
#include "timer.h"

// forward declarations
class Game;
class Player;
struct NET_Server;
struct NET_StreamSocket;

class Server
{
public:
   Server();
   ~Server();

   //! get server instance
   static Server* getInstance();

   //! check if server is listening
   bool isListening() const;

   //! get socket for player id
   NET_StreamSocket* getPlayerSocket(int player_id);

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

   void processStartGameRequest(NET_StreamSocket* tcp_socket, Packet* packet);
   void processJoinGameRequest(NET_StreamSocket* tcp_socket, Packet* packet);
   void processLoginRequest(NET_StreamSocket* tcp_socket, Packet* packet);
   void processListGamesRequest(NET_StreamSocket* tcp_socket);
   void processCreateGameRequest(NET_StreamSocket* tcp_socket, Packet* packet);
   void processGamePacket(NET_StreamSocket* tcp_socket, Packet* packet);
   void processPlayerLeavesGame(NET_StreamSocket* tcp_socket);
   void processPlayerSynchronize(NET_StreamSocket* tcp_socket, Packet* packet);
   void processRemoveGame(int game_id);
   void processRemoveAllBots(int game_id);
   void processBroadcastLeaveGameResponse(Player* player, Game* game);

   //! fix duplicate game names
   void correctDuplicateGameName(Game* new_game);

   //! send single packet
   void sendPacket(NET_StreamSocket* socket, std::unique_ptr<Packet> packet);

   //! accept all pending incoming connections
   void acceptConnections();

   //! read and dispatch all available data for one connected socket
   void readSocket(NET_StreamSocket* tcp_socket);

   //! socket failed or the remote end dropped - clean up and destroy it
   void disconnectSocket(NET_StreamSocket* tcp_socket);

   //! fix duplicate player names
   std::string correctDuplicatePlayerName(const std::string& nick);

   //! player of the given socket, nullptr if unknown
   Player* findPlayer(NET_StreamSocket* socket) const;

   //! listen socket
   NET_Server* _net_server = nullptr;

   //! drives poll() once per tick
   Timer _poll_timer;

   //! per-connection incoming byte buffer
   std::map<NET_StreamSocket*, std::unique_ptr<PacketStreamBuffer>> _socket_buffers;

   //! map of expected packet sizes
   std::map<NET_StreamSocket*, uint16_t> _packet_sizes;

   //! map socket <-> player, owns the players
   std::map<NET_StreamSocket*, std::unique_ptr<Player>> _player_sockets;

   //! map socket <-> game
   std::map<NET_StreamSocket*, Game*> _socket_game_mapping;

   //! map of active games, owns the games
   std::map<int, std::unique_ptr<Game>> _games;

   //! next player id
   int _player_id = 0;

   //! static server instance
   static Server* _instance;

   //! server configuration data
   ServerConfiguration _server_configuration;
};

#endif
