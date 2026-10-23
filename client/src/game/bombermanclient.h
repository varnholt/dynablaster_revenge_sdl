#ifndef BOMBERMANCLIENT_H
#define BOMBERMANCLIENT_H

// shared
#include "gamesignal.h"
#include "nethandles.h"
#include "packetstreambuffer.h"
#include "timer.h"

// framework
#include "framework/keyevent.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

// game
#include <cstdint>
#include "enemytype.h"
#include "gameinformation.h"
#include "playerinfo.h"

// foward declarations
class BotFactory;
class CountdownPacket;
class CreateGameResponsePacket;
class DetonationPacket;
class ErrorPacket;
class ExtraMapItemCreatedPacket;
class ExtraShakePacket;
class GameEventPacket;
class GameStatsPacket;
class JoinGameResponsePacket;
class LeaveGameResponsePacket;
class ListGamesResponsePacket;
class LoginResponsePacket;
class MapItem;
class MapItemCreatedPacket;
class MapItemDestroyedPacket;
class MapItemMovePacket;
class MapItemRemovedPacket;
class MessagePacket;
class Packet;
class PlayerInfectedPacket;
class PlayerKilledPacket;
class Playlist;
class PositionInterpolation;
class PositionPacket;
class Server;
class StartGameResponsePacket;
class StopGameResponsePacket;
class TimePacket;

class BombermanClient
{
public:
   //! constructor
   BombermanClient(/*const std::string& host, const std::string& nick*/);

   //! destructor
   virtual ~BombermanClient();

   //! the client that is alive right now, only valid while hasInstance()
   static BombermanClient& getInstance();

   //! true while a client exists
   static bool hasInstance();

   //! initialize client
   void initialize();

   //! setter for hostname to connect to
   void setHost(const std::string& host);

   //! getter for hostname to connect to
   const std::string& getHost() const;

   //! setter for nickname to use in login
   void setNick(const std::string& nick);

   //! getter for nickname to use in login
   const std::string& getNick() const;

   //! connect to server
   void connectToServer();

   //! immediate login after connecting to server
   void setLoginAfterConnect(bool login_after_connect);

   //! getter for list of games
   std::vector<GameInformation>& getGames();
   const std::vector<GameInformation>& getGames() const;

   //! setter for game id
   void setGameId(int id);

   //! getter for game id
   int getGameId() const;

   //! check if game id is valid
   bool isGameIdValid() const;

   //! getter for game information
   std::optional<std::reference_wrapper<GameInformation>> getGameInformation(int id);
   std::optional<std::reference_wrapper<const GameInformation>> getGameInformation(int id) const;

   //! getter for current game information
   std::optional<std::reference_wrapper<const GameInformation>> getCurrentGameInformation() const;

   //! setter for player id
   void setPlayerId(int);

   //! getter for player id
   int getPlayerId() const;

   //! is player game owner
   bool isPlayerOwner() const;

   //! getter for player color by player id
   Constants::Color getColor(int player_id) const;

   //! get list of players, ordered by id
   std::vector<std::reference_wrapper<const PlayerInfo>> getPlayerInfoList() const;

   //! get map of player
   std::map<int, PlayerInfo>& getPlayerInfoMap();
   const std::map<int, PlayerInfo>& getPlayerInfoMap() const;

   //! add an empty player info to the map, an existing one is replaced
   PlayerInfo& addPlayerInfo(int id);

   //! remove player info
   void removePlayerInfo(int id);

   //! get info object for given player id
   std::optional<std::reference_wrapper<PlayerInfo>> getPlayerInfo(int id);
   std::optional<std::reference_wrapper<const PlayerInfo>> getPlayerInfo(int id) const;

   //! getter for position interpolation
   PositionInterpolation& getPositionInterpolation() const;

   //! setter for the id of the current player info, none if there is no current player
   void setCurrentPlayerId(std::optional<int> id);

   //! getter for current player info
   std::optional<std::reference_wrapper<const PlayerInfo>> getCurrentPlayerInfo() const;

   //! the other players on this machine (see LocalPlayers)
   void setLocalPlayerIds(const std::vector<int>& ids);

   //! true for this client's own player and the other players on this machine
   bool isLocalPlayer(int id) const;

   //! getter for message
   const std::string& getMessage() const;

   //! setter for message
   void setMessage(const std::string& message);

   //! setter for connected state
   void setConnected(bool connected);

   //! check if client is connected
   bool isConnected() const;

   //! check if we're hosting games
   bool isHosting() const;

   //! getter for single/multi player mode
   bool isSinglePlayer() const;

   //! getter for game mode
   Constants::GameMode getGameMode() const;

   //! setter for game mode
   void setGameMode(const Constants::GameMode& mode);

   // communicate with menus

   //! check if main menu is active
   bool isMainMenuActive() const;

public:
   // Signal<> replacements for BombermanClient's former Qt signals (see
   // project_full_qt_removal_scope memory) - every external connect() site now uses these.

   Signal<int, float, float, float> setPlayerPositionSignal;
   Signal<int, float, float, float> setPlayerSpeedSignal;
   Signal<const MapItem&> createMapItemSignal;
   Signal<const MapItem&> removeMapItemSignal;
   Signal<const MapItem&, float> destroyMapItemSignal;
   Signal<int, int, int, int, int, int, float> detonationSignal;
   Signal<int, const std::string&, Constants::Color> addPlayerSignal;
   Signal<int> removePlayerSignal;
   Signal<int> playerIdSignal;
   Signal<float, float> playfieldScaleSignal;
   Signal<int, int> playfieldSizeSignal;
   Signal<const std::string&> loadLevelSignal;
   Signal<const MapItem&> shakeBlockSignal;
   Signal<int, Constants::SkullType, int, int, int> playerInfectedSignal;
   Signal<> connectedSignal;
   Signal<> disconnectedSignal;
   Signal<bool> loginResponseSignal;
   Signal<bool, int, bool> createGameResponseSignal;
   Signal<bool> joinGameResponseSignal;
   Signal<> gameStartedSignal;
   Signal<> gameStoppedSignal;
   Signal<int, const std::string&, bool> messageReceivedSignal;
   Signal<int> countdownSignal;
   Signal<const std::map<int, PlayerInfo>&> playerInfoMapUpdatedSignal;
   Signal<> showGameSignal;
   Signal<> showMenuSignal;
   Signal<> showMainMenuSignal;
   Signal<int, int, bool, Constants::ExtraType, int> extraRemovedSignal;

   Signal<bool> zoomInSignal;
   Signal<bool> zoomOutSignal;
   Signal<const MapItem&, Constants::Direction, float, int, int> moveMapItemSignal;
   Signal<const std::vector<GameInformation>&> gamesListUpdatedSignal;
   Signal<float, int> rumbleSignal;
   Signal<int, int> timeChangedSignal;
   Signal<bool> hostingSignal;

   // story mode
   Signal<int, EnemyType, float, float, float> enemyCreatedSignal;
   Signal<int, float, float, float, float, float, int> enemyPositionSignal;
   Signal<int, bool> enemyKilledSignal;
   Signal<int, int> enemyHitSignal;
   Signal<int, int, int, int, int> storyStateSignal;
   Signal<> leaveGameSignal;

public:
   // key event handlers

   //! process key pressed event handed from gui
   void keyPressed(const KeyEvent&);

   //! process key released event handed from gui
   void keyReleased(const KeyEvent&);

   //! release all keys
   void releaseAllKeys();

   //! process key pressed (not given as key event)
   void processKeyPressed(int key);

   //! process key released
   void processKeyReleased(int key);

   // game workflow

   //! try to log in
   void login(const std::string& nick = "developer");

   //! ask the server to stop the game
   void stopGame();

   //! list games
   void listGames();

   //! create a game
   void createGame(
      const std::string& name,
      const std::string& level,
      int rounds,
      int duration,
      int maxPlayers,
      bool extraBombEnabled,
      bool extraFlameEnabled,
      bool extraSpeedupEnabled,
      bool extraKickEnabled,
      bool extraSkullsEnabled,
      Constants::Dimension dimension
   );

   //! automatically create a game
   void createGameAutomatic();

   //! join a game
   void joinGame(int game = 1);

   //! renames the logged-in player by logging in again, without the menu reacting to it
   void rename(const std::string& nick);

   //! the color asked for when joining, the server assigns another one if it's taken
   void setPreferredColor(std::optional<Constants::Color> color);

   //! start a game
   void startGame(int game = 1);

   //! creates a story mode game beginning at the given stage (0..63)
   void createStoryGame(int first_stage);

   //! the current game is a story mode game
   bool isStory() const;

   //! stop a game
   void stopGame(int game);

   //! send a message to others
   void sendMessage(const std::string& message, bool finishedTyping, int receiverId = -1);

   //! host a game
   void host();

   //! initialize bots
   void initializeBots();

   // functions for communication from or to the menu

   //! process login request
   void loginRequest(const std::string& host, const std::string& nick);

   //! game list request
   void gameListRequest();

   //! request leave game
   void leaveGameRequest();

   //! player is idle
   void idle(bool idle);

   //! level loaded
   void levelLoaded(const std::string& path);

   //! menu page changed
   void setMainMenuActive(bool active);

   //! show ip addresses
   void showIps();

private:
   //! poll for connection progress and incoming data, once per tick
   void poll();

   //! connect client
   void clientConnect();

   //! disconnect client
   void clientDisconnect();

   //! process a packet
   void processPacket(const Packet& packet);

   //! demo mode is finished
   void playbackFinished();

   //! game state changed
   void gameStateChanged();

   //! clear list with player info
   void clearPlayerInfoMap();

private:
   void processCreateGameResponse(const CreateGameResponsePacket& response);
   void processJoinGameResponse(const JoinGameResponsePacket& response);
   void processListGameResponse(const ListGamesResponsePacket& list);
   void processLoginResponse(const LoginResponsePacket& login);
   void processPlayerKilled(const PlayerKilledPacket& kill);
   void processPlayerInfected(const PlayerInfectedPacket& infected_packet);
   void processDetonation(const DetonationPacket& det);
   void processError(const ErrorPacket& error_packet);
   void processPosition(const PositionPacket& pos_packet);
   void processMapItemCreated(const MapItemCreatedPacket& packet);
   void processMapItemMove(const MapItemMovePacket& move_packet);
   void processExtraMapItemCreated(const ExtraMapItemCreatedPacket& packet);
   void processGameStats(const GameStatsPacket& stats_packet);
   void processExtraMapItemDestroyed(const MapItemDestroyedPacket& remove);
   void processMapItemRemoved(const MapItemRemovedPacket& remove);
   void processStartGameResponse(const StartGameResponsePacket& response);
   void processStopGameResponse(const StopGameResponsePacket& response);
   void processGameEvent(const GameEventPacket& response);
   void processMessage(const MessagePacket& message_packet);
   void processTime(const TimePacket& time_packet);
   void processCountdown(const CountdownPacket& countdown_packet);
   void processLeaveGameResponse(const LeaveGameResponsePacket& leave_packet);
   void processExtraShake(const ExtraShakePacket& shake_packet);

   //! send a packet
   void send(Packet& packet);

   //! check for packets
   bool packetAvailable();

   //! read and dispatch all available data from the socket
   void readData();

   //! tear down whatever connection attempt or connection is in progress
   void disconnectFromServer();

   //! show a generic connection-failure message to the user
   void reportConnectionError(std::string_view reason);

   //! get mapitem by mapitem id
   std::optional<std::reference_wrapper<MapItem>> getMapItem(int id) const;

   //! remove all map items, telling everyone about it
   void clearMapItems();

   //! broadcast data about added players
   void broadcastAddPlayerData();

   //! broadcast player start positions
   void broadcastPlayerStartPositions();

   //! send current keys pressed
   void sendKeysPressedPacket();

   //! remove bomb key flag from key pressed combination
   void removeBombKeyFlag();

   // ingame messaging

   //! getter for ingame messaging flag
   bool isIngameMessagingActive() const;

   //! setter for ingame messaging flag
   void setIngameMessagingActive(bool active);

   //! toggle ingame messaging
   void toggleIngameMessaging();

   //! debug keyboard input
   void debugKeyboardInput();

   //! reset client after disonnect or error
   void resetGameData();

   //! reset client state
   void resetClientState();

   //! init playback
   void initializePlayback();

   //! list network devices
   std::vector<std::string> getLocalIps() const;

   // members

   //! keys currently pressed
   int _keys_pressed = 0;

   //! flag indicating bomb key was released
   bool _bomb_released = true;

   //! stream socket to server, empty unless connected or connecting
   NetStreamSocketHandle _socket{nullptr, &NET_DestroyStreamSocket};

   //! host address pending resolution, empty once resolved (or if not resolving)
   NetAddressHandle _address{nullptr, &NET_UnrefAddress};

   //! drives poll() once per tick
   Timer _poll_timer;

   //! incoming byte buffer
   PacketStreamBuffer _buffer;

   //! blocksize of packet which is received from server
   uint16_t _block_size = 0;

   //! player id
   int _id = -1;

   //! game id
   int _game_id = -1;

   //! player alive
   bool _dead = true;

   //! map items by unique id
   std::unordered_map<int, std::unique_ptr<MapItem>> _map_items;

   //! host name
   std::string _host;

   //! the other players on this machine
   std::vector<int> _local_player_ids;

   //! the next login response only confirms a rename
   bool _renaming = false;

   //! color asked for when joining
   std::optional<Constants::Color> _preferred_color;

   //! nick name
   std::string _nick;

   //! list of games available
   std::vector<GameInformation> _games;

   //! connected flag
   bool _connected = false;

   //! login is requested after connect to host
   bool _login_after_connect = false;

   //! server
   std::unique_ptr<Server> _server;

   //! server's own tick thread
   std::jthread _server_thread;

   //! ingame message to send
   std::string _message;

   //! map id <-> player info object
   std::map<int, PlayerInfo> _player_info;

   //! id of the current player info in _player_info
   std::optional<int> _current_player_id;

   //! the client that is alive right now
   static std::optional<std::reference_wrapper<BombermanClient>> _instance;

   //! position interpolation
   std::unique_ptr<PositionInterpolation> _position_interpolation;

   //! typing activated
   bool _ingame_messaging_active = false;

   //! main menu active
   bool _main_menu_active = false;

   //! bot factory
   std::unique_ptr<BotFactory> _bot_factory;

   //! game mode
   Constants::GameMode _game_mode;
};

#endif
