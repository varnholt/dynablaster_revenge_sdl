#ifndef BOTCLIENT_H
#define BOTCLIENT_H

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <vector>

// shared
#include "elapsedtimer.h"
#include "gameinformation.h"
#include "gamesignal.h"
#include "nethandles.h"
#include "packetstreambuffer.h"
#include "serverconfiguration.h"
#include "timer.h"

// forward declarations
class Bot;
class BotPlayerInfo;
class CountdownPacket;
class ExtraMapItemCreatedPacket;
class ExtraShakePacket;
class GameEventPacket;
class JoinGameResponsePacket;
class LeaveGameResponsePacket;
class ListGamesResponsePacket;
class LoginResponsePacket;
class MapItem;
class MapItemCreatedPacket;
class MapItemDestroyedPacket;
class MapItemMovePacket;
class MapItemRemovedPacket;
class Packet;
class PlayerInfectedPacket;
class PlayerKilledPacket;
class PositionPacket;
class StartGameResponsePacket;
class StopGameResponsePacket;

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
   std::optional<std::reference_wrapper<BotPlayerInfo>> getPlayerInfo(int id) const;

   //! getter for the map of players
   const PlayerInfoMap& getPlayerInfoMap() const;

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
   Signal<const std::shared_ptr<MapItem>&> mapItemCreatedSignal;

   //! a map item has been removed
   Signal<const MapItem&> mapItemRemovedSignal;

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
   void processLoginResponse(const LoginResponsePacket& login_response);

   //! join game response
   void processJoinGameResponse(const JoinGameResponsePacket& response);

   //! leave game response
   void processLeaveGameResponse(const LeaveGameResponsePacket& response);

   //! process position packet
   void processPosition(const PositionPacket& position_packet);

   //! a map item has been created
   void processMapItemCreated(const MapItemCreatedPacket& created_packet);

   //! a map item has been destroyed
   void processMapItemRemoved(const MapItemRemovedPacket& removed_packet);

   //! a map item is moved (kicked)
   void processMapItemMove(const MapItemMovePacket& move_packet);

   //! an extra map item has been created
   void processExtraMapItemCreated(const ExtraMapItemCreatedPacket& created_packet);

   //! an extra map item has been destroyed
   void processMapItemDestroyed(const MapItemDestroyedPacket& destroyed_packet);

   //! game was started
   void processStartGameResponse(const StartGameResponsePacket& response);

   //! game event received
   void processGameEvent(const GameEventPacket& game_event_packet);

   //! player infected packet received
   void processPlayerInfected(const PlayerInfectedPacket& infected_packet);

   //! player killed packet received
   void processPlayerKilled(const PlayerKilledPacket& killed_packet);

   //! game stopped
   void processStopGameResponse(const StopGameResponsePacket& stop_game_packet);

   //! list games
   void processListGameResponse(const ListGamesResponsePacket& list);

   //! process countdown
   void processCountdown(const CountdownPacket& countdown_packet);

   //! process extra shake
   void processExtraShake(const ExtraShakePacket& extra_shake_packet);

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
   void queueObsoleteItem(int id);

   //! clear obsolete items
   void clearObsoleteItems();

   //! getter for map item by id, empty if unknown
   std::shared_ptr<MapItem> getMapItem(int id) const;

   //! add a map item
   void addMapItem(std::shared_ptr<MapItem> map_item);

   //! send a packet
   void send(Packet& packet);

   //! check for packets
   bool packetAvailable();

   //! read and dispatch all available data from the socket
   void readData();

   //! create a new map
   void createMap(Constants::Dimension dimensions);

   //! initialize a bot map
   void initBotMap(int width, int height);

   //! getter for bot
   Bot& getBot() const;

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

   //! stream socket to server, empty unless connected or connecting
   NetStreamSocketHandle _socket{nullptr, &NET_DestroyStreamSocket};

   //! host address pending resolution, empty once resolved (or if not resolving)
   NetAddressHandle _address{nullptr, &NET_UnrefAddress};

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

   //! map items by id, shared with the bot map
   std::map<int, std::shared_ptr<MapItem>> _map_items;

   //! queue of items to be removed from the bot map later
   std::queue<std::shared_ptr<MapItem>> _obsolete_map_items;

   //! map id <-> player info object
   PlayerInfoMap _player_info;

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
