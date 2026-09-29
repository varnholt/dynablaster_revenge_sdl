#ifndef BOTCLIENT_H
#define BOTCLIENT_H

#include <map>
#include <queue>

// shared
#include "elapsedtimer.h"
#include "gameinformation.h"
#include "packetstreambuffer.h"
#include "serverconfiguration.h"
#include "signal.h"
#include "timer.h"

#include <string>
#include <vector>

// forward declarations
class Bot;
class BotMap;
class MapItem;
class Packet;
class BotPlayerInfo;
struct NET_Address;
struct NET_StreamSocket;

class BotClient
{
public:
   //! constructor
   BotClient();

   //! destructor
   virtual ~BotClient();

   //! initialize bot client
   void initialize();

   //! connect to server
   void connectToServer();

   void initializeAutoJoinStart();

   //! setter for host name
   void setHost(const std::string& host);

   //! setter for nickname to use in login
   void setNick(const std::string& nick);

   //! setter for bot
   void setBot(Bot* bot);

   //! setter for bot map
   void setBotMap(BotMap* map);

   //! setter for autojoin
   void setAutoJoin(bool enabled);

   //! setter for autostart
   void setAutoStart(bool enabled);

   //! setter for game id
   void setGameId(int id);

   //! getter for game id
   int getGameId() const;

   //! get info object for given player id
   BotPlayerInfo* getPlayerInfo(int id) const;

   //! get a list of players
   std::vector<BotPlayerInfo*> getPlayerInfoList() const;

   //! getter for the map of players
   std::map<int, BotPlayerInfo*>* getPlayerInfoMap();

   //! setter for server configuration
   void setServerConfiguration(const ServerConfiguration&);

   //! getter for server configuration
   const ServerConfiguration& getServerConfiguration() const;

   //! getter for delta x
   float getDeltaX() const;

   //! setter for delta x
   void setDeltaX(float value);

   //! getter for delta y
   float getDeltaY() const;

   //! setter for delta y
   void setDeltaY(float value);

   //! player logged in
   Signal<int> updatePlayerIdSignal;

   //! a map item has been created
   Signal<MapItem*> mapItemCreatedSignal;

   //! a map item has been removed
   Signal<MapItem*> mapItemRemovedSignal;

   //! game selected
   Signal<> gameSelectedSignal;

   //! game started
   Signal<> gameStartedSignal;

   //! update player position
   Signal<int, float, float, float> updatePlayerPositionSignal;

   //! bot shall be removed
   Signal<> removeSignal;

   //! extra shake
   Signal<int> extraShakeSignal;

   //! mark temporary hazardous
   Signal<int, int, int, int> markHazardousTemporarySignal;

   //! make hazardous temp for bomb kicks
   Signal<int, int, Constants::Direction, int> bombKickedSignal;

public:

   //! send walk packet
   void walk(int8_t);

   //! drop a bomb
   void bomb();

   //! send a message to others
   void sendMessage(const std::string& message, bool finished_typing = true, int receiver_id = -1);

   //! delete obsolete items
   void deleteObsoleteMapItems();

private:

   //! poll for connection progress and incoming data, once per tick
   void poll();

   //! connect client
   void clientConnect();

   //! disconnect client
   void clientDisconnect();

   //! login
   void login();

   //! select game
   void selectGame();

   //! join a game
   void joinGame();

   //! start game
   void startGame();

   // packet handlers

   //! login response
   void processLoginResponse(Packet* p);

   //! join game response
   void processJoinGameResponse(Packet* p);

   //! leave game response
   void processLeaveGameResponse(Packet* p);

   //! process position packet
   void processPosition(Packet* packet);

   //! a map item has been created
   void processMapItemCreated(Packet* packet);

   //! a map item has been destroyed
   void processMapItemRemoved(Packet* packet);

   //! a map item is moved (kicked)
   void processMapItemMove(Packet* packet);

   //! an extra map item has been created
   void processExtraMapItemCreated(Packet* packet);

   //! an extra map item has been destroyed
   void processMapItemDestroyed(Packet* packet);

   //! game was started
   void processStartGameResponse(Packet* packet);

   //! game event received
   void processGameEvent(Packet* packet);

   //! player infected packet received
   void processPlayerInfected(Packet* packet);

   //! player killed packet received
   void processPlayerKilled(Packet* packet);

   //! game stopped
   void processStopGameResponse(Packet* packet);

   //! list games
   void processListGameResponse(Packet* packet);

   //! process countdown
   void processCountdown(Packet* packet);

   //! process extra shake
   void processExtraShake(Packet* packet);

   //! setter for game joined flag
   void setGameJoined(bool joined);

   //! getter for game joined flag
   bool isGameJoined() const;

   //! setter for player speed
   void setSpeed(float speed);

   //! getter for player speed
   float getSpeed() const;

   //! getter for player id
   int getPlayerId() const;

private:
   //! queue delete item
   void queueObsoleteItem(MapItem* item);

   //! clear obsolete items
   void clearObsoleteItems();

   //! getter for map item by id
   MapItem* getMapItem(int id) const;

   //! add a map item
   void addMapItem(MapItem* map_item);

   //! remove a map item
   void removeMapItem(MapItem* map_item);

   //! send a packet
   void send(Packet* packet);

   //! check for packets
   bool packetAvailable();

   //! read and dispatch all available data from the socket
   void readData();

   //! create a new map
   void createMap(Constants::Dimension dimensions);

   //! initialize a bot map
   void initBotMap(int width, int height);

   //! getter for bot ptr
   Bot* getBot() const;

   //! reset bot
   void resetBot();

   //! track a bug
   void bugTrack1();

   //! getter for walk count
   int getWalkCount() const;

   //! increase walk count
   void increaseWalkCount();

   //! reset walk count
   void resetWalkCount();

   //! stream socket to server, null unless connected or connecting
   NET_StreamSocket* _socket;

   //! host address pending resolution, null once resolved (or if not resolving)
   NET_Address* _address;

   //! drives poll() once per tick
   Timer _poll_timer;

   //! incoming byte buffer
   PacketStreamBuffer _buffer;

   //! host name
   std::string _host;

   //! nick name
   std::string _nick;

   //! connected flag
   bool _connected;

   //! expected block size of current packet
   uint16_t _block_size;

   //! bot
   Bot* _bot;

   //! botmap
   BotMap* _bot_map;

   //! game id to join
   int _game_id;

   //! automatically join game
   bool _auto_join;

   //! automatically start game
   bool _auto_start;

   //! player id
   int _player_id;

   //! map items
   std::map<int, MapItem*> _map_items;

   //! map id <-> player info object
   std::map<int, BotPlayerInfo*> _player_info;

   //! bot's keyboard keys pressed
   int _keys_pressed;

   //! list of games available
   mutable std::vector<GameInformation> _games;

   //! true if game was succesfully joined
   bool _game_joined;

   //! current speed
   float _speed;

   //! queue of items to be deleted later
   std::queue<MapItem*> _obsolete_map_items;

   //! server configuration
   ServerConfiguration _server_configuration;

   //! time elapsed since last bomb
   ElapsedTimer _bomb_time;

   //! walk counter
   int _walk_count;

   //! delta x
   float _delta_x;

   //! delta y
   float _delta_y;
};

#endif  // BOTCLIENT_H
