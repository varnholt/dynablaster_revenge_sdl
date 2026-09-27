#ifndef SERVER_H
#define SERVER_H

#include <map>
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

   //! constructor
   Server();

   //! destructor
   ~Server();

   //! get server instance
   static Server* getInstance();

   //! check if server is listening
   bool isListening() const;

   //! get socket for player id
   NET_StreamSocket* getPlayerSocket( int playerId );

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

   void processStartGameRequest(NET_StreamSocket* tcpSocket, Packet* packet);
   void processJoinGameRequest(NET_StreamSocket* tcpSocket, Packet* packet);
   void processLoginRequest(NET_StreamSocket* tcpSocket, Packet* packet);
   void processListGamesRequest(NET_StreamSocket* tcpSocket);
   void processCreateGameRequest(NET_StreamSocket* tcpSocket, Packet* packet);
   void processGamePacket(NET_StreamSocket* tcpSocket, Packet* packet);
   void processPlayerLeavesGame(NET_StreamSocket* socket);
   void processPlayerSynchronize(NET_StreamSocket* tcpSocket, Packet* packet);
   void processRemoveGame(int gameId);
   void processRemoveAllBots(int gameId);
   void processBroadcastLeaveGameResponse(Player* player, Game* game);

   //! fix duplicate game names
   void correctDuplicateGameName(Game* game);

   //! send single packet
   void sendPacket(NET_StreamSocket* socket, Packet* packet);

   //! accept all pending incoming connections
   void acceptConnections();

   //! read and dispatch all available data for one connected socket
   void readSocket(NET_StreamSocket* socket);

   //! socket failed or the remote end dropped - clean up and destroy it
   void disconnectSocket(NET_StreamSocket* socket);

   //! fix duplicate player names
   std::string correctDuplicatePlayerName(const std::string& nick);

   //! listen socket
   NET_Server* mNetServer;

   //! drives poll() once per tick
   Timer mPollTimer;

   //! per-connection incoming byte buffer
   std::map<NET_StreamSocket*, PacketStreamBuffer*> mSocketBuffers;

   //! map of expected packet sizes
   std::map<NET_StreamSocket*, uint16_t> mPacketSizes;

   //! map socket <-> player
   std::map<NET_StreamSocket*, Player*> mPlayerSockets;

   //! map socket <-> game
   std::map<NET_StreamSocket*, Game*> mSocketGameMapping;

   //! map of active games
   std::map<int, Game*> mGames;

   //! next player id
   int mPlayerId;

   //! static server instance
   static Server* sInstance;

   //! server configuration data
   ServerConfiguration mServerConfiguration;
};


#endif
