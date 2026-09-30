#ifndef BOTCLIENT_H
#define BOTCLIENT_H

#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <string>
#include <vector>

// shared
#include "elapsedtimer.h"
#include "gameinformation.h"
#include "packetstreambuffer.h"
#include "serverconfiguration.h"
#include "gamesignal.h"
#include "timer.h"

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
   using PlayerInfoMap = std::map<int, std::unique_ptr<BotPlayerInfo>>;

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

   //! setter for bot, the client takes ownership
   void setBot(std::unique_ptr<Bot> bot);

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
   PlayerInfoMap* getPlayerInfoMap();

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
   void processLoginResponse(Packet* packet);

   //! join game response
   void processJoinGameResponse(Packet* packet);

   //! leave game response
   void processLeaveGameResponse(Packet* packet);

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

   //! queue delete item
   void queueObsoleteItem(MapItem* item);

   //! clear obsolete items
   void clearObsoleteItems();

   //! getter for map item by id
   MapItem* getMapItem(int id) const;

   //! add a map item
   void addMapItem(std::unique_ptr<MapItem> map_item);

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

   //! take all map items out of the bot map without deleting them (they are owned by _map_items)
   void detachBotMapItems();

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
   NET_StreamSocket* _socket = nullptr;

   //! host address pending resolution, null once resolved (or if not resolving)
   NET_Address* _address = nullptr;

   //! drives poll() once per tick
   Timer _poll_timer;

   //! incoming byte buffer
   PacketStreamBuffer _buffer;

   //! host name
   std::string _host;

   //! nick name
   std::string _nick;

   //! connected flag
   bool _connected = false;

   //! expected block size of current packet
   uint16_t _block_size = 0;

   //! map items, the bot map only holds non-owning pointers to these
   std::map<int, std::unique_ptr<MapItem>> _map_items;

   //! queue of items to be deleted later
   std::queue<std::unique_ptr<MapItem>> _obsolete_map_items;

   //! map id <-> player info object
   PlayerInfoMap _player_info;

   //! botmap
   std::unique_ptr<BotMap> _bot_map;

   //! bot
   std::unique_ptr<Bot> _bot;

   //! game id to join
   int _game_id = 0;

   //! automatically join game
   bool _auto_join = true;

   //! automatically start game
   bool _auto_start = false;

   //! player id
   int _player_id = 0;

   //! bot's keyboard keys pressed
   int _keys_pressed = 0;

   //! list of games available
   mutable std::vector<GameInformation> _games;

   //! true if game was succesfully joined
   bool _game_joined = false;

   //! current speed
   float _speed = 0.0f;

   //! server configuration
   ServerConfiguration _server_configuration;

   //! time elapsed since last bomb
   ElapsedTimer _bomb_time;

   //! walk counter
   int _walk_count = 0;

   //! delta x
   float _delta_x = 0.0f;

   //! delta y
   float _delta_y = 0.0f;
};

#endif  // BOTCLIENT_H
