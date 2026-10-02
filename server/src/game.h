#ifndef GAME_H
#define GAME_H

#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

// shared
#include "constants.h"
#include "creategamedata.h"
#include "elapsedtimer.h"
#include "gameinformation.h"
#include "gameround.h"
#include "packet.h"
#include "gamesignal.h"
#include "timer.h"

// forward declarations
class BombMapItem;
class CollisionDetection;
class ExtraMapItem;
class ExtraShakePacketHandler;
class ExtraSpawn;
class Map;
class MapItem;
class MapItemCreatedPacket;
class Player;
class PlayerDisease;
struct NET_StreamSocket;

class Game
{
public:
   Game();
   ~Game();

   //! initialize everything
   void initialize();

   //! getter for the game's id
   int getId() const;

   //! getter for the player count
   int getPlayerCount() const;

   //! getter for the maximum player count
   int getMaximumPlayerCount() const;

   //! setter for create game data
   void setCreateGameData(const CreateGameData& data);

   //! setter for game name
   void setName(const std::string& name);

   //! getter for the game's name
   const std::string& getName() const;

   //! getter for the level's name
   const std::string& getLevelName() const;

   //! process a packet
   void processPacket(NET_StreamSocket* tcp_socket, Packet* packet);

   //! getter for map socket <-> player
   const std::map<NET_StreamSocket*, Player*>& getPlayerSockets() const;

   //! socket of the given player, nullptr if the player is not in this game
   NET_StreamSocket* getSocket(Player* player) const;

   //! setter for the game's creator
   void setCreator(Player* creator);

   //! getter for the game's creator
   Player* getCreator() const;

   //! getter for list of players
   std::vector<Player*> getPlayers() const;

   //! getter for the game's map
   Map* getMap() const;

   //! getter for map dimension
   Constants::Dimension getMapDimension() const;

   //! get combined extra enum
   int getExtras() const;

   //! getter for game duration
   int getDuration() const;

   //! broadcast a message to all players in the game
   void broadcastMessage(const std::string& message);

   //! broadcast start game
   void broadcastStartGame();

   //! check gameover condition
   void updateGameoverCondition();

   //! getter for rounds played
   int getGamesPlayed() const;

   //! check if game is only populated by bots
   bool isGamePopulatedByBots() const;

   //! getter for free player color, the preferred one if it's free
   Constants::Color getColorForNextPlayer(std::optional<Constants::Color> preferred_color = std::nullopt) const;

   //! getter for game information object
   GameInformation getGameInformation();

   //! getter for the current game state
   Constants::GameState getState() const;

   //! set synchronization active
   void setSynchronizationActive(bool active);

   //! return synchronization flag
   bool isSynchronizationActive() const;

   //! player joins a game
   bool joinGame(Player* player, NET_StreamSocket* player_socket, std::optional<Constants::Color> preferred_color = std::nullopt);

   //! getter for position skip count
   int getPositionSkipCount() const;

   //! get the actual number of bots in this game
   int getBotCount() const;

   //! getter for gameround ptr
   GameRound* getGameRound();

   //! getter for time left
   int getTimeLeft();

   //! check if start positions are initialized
   bool isStartPositionInitialized() const;

   //! set start positions to initialized
   void setStartPositionsInitialized(bool value);

   //! getter for message shown flag
   bool isGameOnlyPopulatedByBotsMessageShown() const;

   //! setter for message shown flag
   void setGameOnlyPopulatedByBotsMessageShown(bool value);

   //! process spectator client
   void processSpectator(NET_StreamSocket* tcp_socket);

   //! start new game
   void startGame();

   //! (force) stop the game
   void stopGame();

   //! start game synchronization
   void startSynchronization();

   //! game synchronization step
   void synchronize();

   //! prepare game
   void prepareGame();

   //! game finished
   void finishGame();

   //! player left the game
   void removePlayer(Player* player, NET_StreamSocket* player_socket);

   //! add packet to list of outgoing packets
   void addOutgoingPacket(std::unique_ptr<Packet> packet);

   //! player was killed
   Signal<int> playerKilledSignal;

   //! player leaves game
   Signal<int> playerLeavesSignal;

   //! player is forced to leave game
   Signal<NET_StreamSocket*> forceLeaveGameSignal;

   //! state changed
   Signal<Constants::GameState> stateChangedSignal;

private:
   //! update everything
   void update();

   //! bomb exploded (newschool, iterative detonations)
   void bombExploded(BombMapItem* bomb, bool);

   //! send game time packets
   void processGameTime();

   //! update prepare game countdown
   void updatePrepareGame();

   //! bomb kick animation triggered
   void bombKickedAnimation(BombMapItem* item, Constants::Direction direction, float speed);

   //! send an idle packet
   void playerIdle(int8_t directions, Player* player);

   //! move player
   void playerMove(Player* player, float assigned_x_position, float assigned_y_position, int8_t directions);

   //! player kicks a bomb
   void playerKicksBomb(Player* player, MapItem* item, bool vertically_kicked, int keys_pressed);

   //! player disease stopped
   void playerDiseaseStopped(PlayerDisease* disease);

   //! send a message to the spectator that the game is currently running
   void processSpectatorMessage();

   //! spawn an extra
   void spawn();

   //! initialize skull setup
   void initSkullSetup();

   //! initialize the map
   void initializeMap();

   //! initialize timers
   void initializeTimers();

   //! initialize players
   void initializePlayerStartPositions();

   //! initialize extra spawn
   void initializeExtraSpawn();

   //! broadcast map
   void broadcastCreateMapItems();

   //! broadcast and clear map
   void broadcastClearMapItems();

   //! broadcast start positions
   void broadcastStartPositions();

   //! update positions
   void updatePositions();

   //! update player positions
   void updatePlayerPositions();

   //! update bombs
   void updateBombs();

   //! check if player collects an extra
   void updateExtras();

   //! check if infected players collide
   void updateInfections();

   //! send all packets from the outgoing-vector
   void sendBroadcastPackets();

   //! send single packet
   void sendPacket(NET_StreamSocket* socket, std::unique_ptr<Packet> packet);

   //! player of the given socket, nullptr if unknown
   Player* findPlayer(NET_StreamSocket* socket) const;

   //! setter for the current game state
   void setState(Constants::GameState state);

   //! update stats on player kill event
   void updateStatsPlayerKilled(Player* killer, Player* victim);

   //! update stats on player won event
   void processPlayerWon(Player* player);

   //! only bots left
   void processOnlyBotsLeft();

   //! broadcast game stats
   void broadcastGameStats();

   //! increase the number of rounds played
   void increaseGamesPlayed();

   //! init all map related items on prepare-game-phase
   void initMapRelatedItems();

   //! check if kicking is possible
   bool isKickPossible(int x, int y, Constants::Direction kick_direction);

   //! next game round
   void nextRound();

   //! reset round stats
   void resetRoundStats();

   //! broadcast game information
   void broadcastGameInformation();

   //! create extra skull
   void createInfection(
      Player* infected_player,
      Player* infecting_player = nullptr,
      ExtraMapItem* extra = nullptr,
      const std::vector<Constants::SkullType>& faces = {}
   );

   //! create kick animation
   void createKickAnimation(BombMapItem* kicked_bomb, Constants::Direction kick_direction);

   //! decrease immune times
   void updateImmuneTimes();

   //! make field immune
   void makeFieldImmune(int x, int y);

   //! check if field is immune
   bool isFieldImmune(int x, int y) const;

   //! a counter checking how many players left the game during the running round
   void increasePlayersLeftTheGameCount();

   //! reset the player left game counter
   void resetPlayersLeftTheGameCount();

   //! read players left game counter
   int getPlayersLeftTheGameCount() const;

   //! send a message to game owner
   void sendMessageToOwner(const std::string& message);

   //! do not rotate dead player into stones
   void rotateDeadPlayerTowardsBomb(Constants::Direction detonation_direction, Player* player, int x, int y);

   //! check if extra spawning is enabled
   bool isSpawnExtrasEnabled() const;

   //! Timer::singleShot() that is dropped if this game has been destroyed in the meantime
   void singleShotWhileAlive(int32_t milliseconds, std::function<void()> callback);

   //! static game id counter
   static int _game_id_counter;

   //! expires with this game (destroyed last); destroy callbacks of diseases/kick animations
   //! outliving it check it before touching the game
   std::shared_ptr<bool> _lifetime = std::make_shared<bool>(true);

   //! calls the update loop
   Timer _update_timer;

   //! map socket <-> player
   std::map<NET_StreamSocket*, Player*> _player_sockets;

   //! map id <-> player - key order matters: start positions are assigned by ascending id
   std::map<int8_t, Player*> _players;

   //! outgoing packages
   std::deque<std::unique_ptr<Packet>> _outgoing_packets;

   //! playfield
   std::unique_ptr<Map> _map;

   //! map items to delete after destruction
   std::unordered_set<MapItem*> _destroyed_map_items;

   //! one direction to check
   std::vector<Constants::Direction> _direction_check_center{Constants::DirectionUp};

   //! all 4 directions to check
   std::vector<Constants::Direction> _direction_check_all{
      Constants::DirectionUp,
      Constants::DirectionDown,
      Constants::DirectionLeft,
      Constants::DirectionRight
   };

   //! game id
   int _game_id = ++_game_id_counter;

   //! game create data
   CreateGameData _create_game_data;

   //! flag to indicate game is running
   bool _running = false;

   //! server time update timer
   Timer _game_time_update_timer;

   //! game time
   ElapsedTimer _game_time;

   //! game owner
   Player* _creator = nullptr;

   //! game state
   Constants::GameState _state = Constants::GameStopped;

   //! preparation timer
   Timer _preparation_timer;

   //! prepration time
   ElapsedTimer _preparation_time;

   //! preparation counter
   int _preparation_counter = 0;

   //! idle packet map
   std::unordered_set<Player*> _idle_packet_sent_set;

   //! skip countdown
   bool _skip_countdown = false;

   //! position skips
   int _position_skip_count = 0;

   //! number of rounds played
   int _games_played = 0;

   //! game synchronization time
   ElapsedTimer _synchronization_time;

   //! synchronization is active or not
   bool _synchronization_active = false;

   //! shake packet handler
   std::unique_ptr<ExtraShakePacketHandler> _shake_packet_handler;

   //! maximum player speed
   float _max_speed = 0.0f;

   //! sync max time
   int _sync_max_time = 0;

   //! collision detection
   std::unique_ptr<CollisionDetection> _collision_detection;

   //! game round instance
   GameRound _game_round;

   //! immune time per field
   std::vector<int32_t> _immune_times;

   //! counter of players left per round
   int _player_left_the_game_count = 0;

   //! start positions are initialized
   bool _start_positions_initialized = false;

   //! only populated message shown flag
   bool _game_only_populated_by_bots_message_shown = false;

   //! list of spectators - cleared explicitly in removePlayer() when a socket goes away
   std::deque<NET_StreamSocket*> _spectators;

   //! extra spawning
   std::unique_ptr<ExtraSpawn> _extra_spawn;

   //! extra spawning enabled
   bool _extra_spawn_enabled = false;
};

#endif
