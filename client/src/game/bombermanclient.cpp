#include "bombermanclient.h"

#include <algorithm>

// bomberman shared
#include "constants.h"

// client and shared
#include "countdownpacket.h"
#include "creategamerequestpacket.h"
#include "creategameresponsepacket.h"
#include "detonationpacket.h"
#include "errorpacket.h"
#include "extramapitem.h"
#include "extramapitemcreatedpacket.h"
#include "extrashakepacket.h"
#include "extrashakepackethandler.h"
#include "gameeventpacket.h"
#include "gamehelptexts.h"
#include "gameplayback.h"
#include "gamesettings.h"
#include "gamestatemachine.h"
#include "gamestatspacket.h"
#include "helpmanager.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "keypacket.h"
#include "leavegamerequestpacket.h"
#include "leavegameresponsepacket.h"
#include "levels/level.h"
#include "listgamesrequestpacket.h"
#include "listgamesresponsepacket.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "mapitemcreatedpacket.h"
#include "mapitemdestroyedpacket.h"
#include "mapitemmovepacket.h"
#include "mapitemremovedpacket.h"
#include "math/vector.h"
#include "messagepacket.h"
#include "playerdisease.h"
#include "playerinfectedpacket.h"
#include "playerkilledpacket.h"
#include "playerstats.h"
#include "playersynchronizepacket.h"
#include "positioninterpolation.h"
#include "positionpacket.h"
#include "soundmanager.h"
#include "startgamerequestpacket.h"
#include "startgameresponsepacket.h"
#include "stopgamerequestpacket.h"
#include "stopgameresponsepacket.h"
#include "stringutils.h"
#include "timepacket.h"
#include "tools/stream.h"

#include <chrono>
#include <format>
#include <string>
#include <thread>

// ai
#include "botfactory.h"

// server
#include "server.h"

// qt
#include "logging.h"

// SDL
#include <SDL3/SDL_keycode.h>
#include <SDL3_net/SDL_net.h>

#include <cstdint>

// static variables
BombermanClient* BombermanClient::_instance = nullptr;

//-----------------------------------------------------------------------------
/*!
 */
BombermanClient::BombermanClient(/*const std::string& host, const std::string& nick*/)
{
   _instance = this;

   _position_interpolation = std::make_unique<PositionInterpolation>();

   _position_interpolation->bounceSignal.connect([]() { SoundManager::getInstance()->playSoundBombBounce(); });

   GameStateMachine::getInstance()->stateChangedSignal.connect([this]() { gameStateChanged(); });

   _bot_factory = std::make_unique<BotFactory>();

   // enable game playback
   initializePlayback();
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::initialize()
{
   _poll_timer.timeoutSignal.connect([this]() { poll(); });
   _poll_timer.start(16);

   // interpolation
   moveMapItemSignal.connect(
      [this](MapItem* item, Constants::Direction dir, float speed, int nominalX, int nominalY)
      { _position_interpolation->moveMapItem(item, dir, speed, nominalX, nominalY); }
   );
}

//-----------------------------------------------------------------------------
/*!
 */
BombermanClient::~BombermanClient()
{
   _instance = nullptr;
   clearPlayerInfoMap();

   _server_thread.request_stop();
   if (_server_thread.joinable())
   {
      _server_thread.join();
   }

   // only after the server thread stopped using it
   _server.reset();
}

//-----------------------------------------------------------------------------
/*!
   \return singleton instance of client
*/
BombermanClient* BombermanClient::getInstance()
{
   return _instance;
}

//-----------------------------------------------------------------------------
/*!
   \return ptr to list of games
*/
std::vector<GameInformation>* BombermanClient::getGames() const
{
   return &_games;
}

//-----------------------------------------------------------------------------
/*!
   \param id game id
*/
void BombermanClient::setGameId(int id)
{
   _game_id = id;
}

//-----------------------------------------------------------------------------
/*!
   \return game id
*/
int BombermanClient::getGameId() const
{
   return _game_id;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if game is valid
*/
bool BombermanClient::isGameIdValid() const
{
   return getGameId() != -1;
}

//-----------------------------------------------------------------------------
/*!
   \return game information
*/
GameInformation* BombermanClient::getGameInformation(int id) const
{
   const auto game = std::ranges::find_if(_games, [id](const GameInformation& info) { return info.getId() == id; });
   return (game != _games.end()) ? &*game : nullptr;
}

//-----------------------------------------------------------------------------
/*!
   \return game information
*/
GameInformation* BombermanClient::getCurrentGameInformation() const
{
   return getGameInformation(getGameId());
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
*/
void BombermanClient::setPlayerId(int id)
{
   _id = id;
}

//-----------------------------------------------------------------------------
/*!
   \return player id
*/
int BombermanClient::getPlayerId() const
{
   return _id;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if player is owner
*/
bool BombermanClient::isPlayerOwner() const
{
   bool owner = false;
   GameInformation* info = getCurrentGameInformation();

   if (info)
   {
      owner = info->getCreatorId() == getPlayerId();
   }

   return owner;
}

//-----------------------------------------------------------------------------
/*!
   \param player_id player id
   \return player color
*/
Constants::Color BombermanClient::getColor(int player_id) const
{
   Constants::Color color = Constants::ColorWhite;

   PlayerInfo* info = getPlayerInfo(player_id);

   if (info)
      color = info->getColor();

   return color;
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to send
*/
void BombermanClient::send(Packet* packet)
{
   packet->serialize();

   if (_socket)
   {
      NET_WriteToStreamSocket(_socket, packet->constData(), static_cast<int>(packet->size()));
   }
}

//-----------------------------------------------------------------------------
/*!
   \param host host name
*/
void BombermanClient::setHost(const std::string& host)
{
   _host = host;
}

//-----------------------------------------------------------------------------
/*!
   \return host name
*/
const std::string& BombermanClient::getHost() const
{
   return _host;
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::connectToServer()
{
   _address = NET_ResolveHostname(getHost().c_str());

   if (!_address)
   {
      reportConnectionError(SDL_GetError());
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::clientConnect()
{
   qDebug("BombermanClient::clientConnect()");

   setConnected(true);

   if (_login_after_connect)
   {
      login(getNick());
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::clientDisconnect()
{
   if (isConnected())
   {
      // 1) show menu
      // 2) open main screen
      // 3) display disconnected message

      // we were disconnected
      disconnectedSignal();
   }
   else
   {
      // we had to disconnect first before reconnecting to
      // another server
      loginRequest(getHost(), getNick());
   }

   resetGameData();
   resetClientState();

   qDebug("disconnected");

   auto it = _map_items.begin();
   while (it != _map_items.end())
   {
      MapItem* item = it->second;
      it = _map_items.erase(it);
      delete item;
   }
}

//-----------------------------------------------------------------------------
/*!
   tear down whatever connection attempt or connection is in progress
*/
void BombermanClient::disconnectFromServer()
{
   if (_address)
   {
      NET_UnrefAddress(_address);
      _address = nullptr;
   }

   if (_socket)
   {
      NET_DestroyStreamSocket(_socket);
      _socket = nullptr;
   }

   clientDisconnect();
}

//-----------------------------------------------------------------------------
/*!
  \param reason low-level failure reason, from SDL_GetError()
*/
void BombermanClient::reportConnectionError(const char* reason)
{
   std::string message = TEXT_ERROR_NETWORK_GENERAL;

   if (reason && *reason)
   {
      message += std::format(" ({})", reason);
   }

   HelpManager::getInstance()->addMessage("", message, Constants::HelpSeverityError);
}

//-----------------------------------------------------------------------------
/*!
   \param in datastream
   \return true if sufficient data was received
*/
bool BombermanClient::packetAvailable()
{
   // blocksize not initialized yet
   if (_block_size == 0)
   {
      // not enough data to read blocksize?
      if (_buffer.bytesAvailable() < sizeof(uint16_t))
         return false;

      // read blocksize
      BinaryReader size_reader = _buffer.reader();
      size_reader >> _block_size;
      _buffer.consume(size_reader.pos());
   }

   // enough data?
   return (_buffer.bytesAvailable() >= _block_size);
}

//-----------------------------------------------------------------------------
/*!
   \return ingame messaging active flag
*/
bool BombermanClient::isIngameMessagingActive() const
{
   return _ingame_messaging_active;
}

//-----------------------------------------------------------------------------
/*!
   \param active ingame messaging active flag
*/
void BombermanClient::setIngameMessagingActive(bool active)
{
   _ingame_messaging_active = active;
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::toggleIngameMessaging()
{
   setIngameMessagingActive(!isIngameMessagingActive());
}

//-----------------------------------------------------------------------------
/*!
   \param id id of the map id
   \param mapitem map item ptr
*/
MapItem* BombermanClient::getMapItem(int id) const
{
   MapItem* item = nullptr;

   auto it = _map_items.find(id);
   if (it != _map_items.end())
      item = it->second;

   return item;
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processCreateGameResponse(Packet* packet)
{
   CreateGameResponsePacket* response = dynamic_cast<CreateGameResponsePacket*>(packet);

   // extract game information
   GameInformation game_information = response->getGameInformation();
   int game_id = game_information.getId();
   int player_id = game_information.getCreatorId();
   bool success = false;

   if (game_id > -1)
   {
      qDebug("BombermanClient::data(): game %d created", game_id);

      // update game information
      GameInformation* old_game_information = getGameInformation(game_id);
      if (old_game_information)
      {
         *old_game_information = game_information;
      }
      else
      {
         _games.push_back(game_information);
      }

      success = true;
   }
   else
   {
      qDebug("BombermanClient::data(): game create request failed");
   }

   createGameResponseSignal(success, game_id, player_id == getPlayerId());
}

//-----------------------------------------------------------------------------
/*!
   \param id key
   \param info value
*/
void BombermanClient::addPlayerInfo(int id, PlayerInfo* info)
{
   _player_info[id] = info;
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processJoinGameResponse(Packet* packet)
{
   JoinGameResponsePacket* response = dynamic_cast<JoinGameResponsePacket*>(packet);

   if (response->isSuccessful())
   {
      PlayerInfo* info = nullptr;

      int id = response->getPlayerId();

      if (!_player_info.contains(id))
      {
         info = new PlayerInfo();
         info->setId(id);
         info->setNick(response->getNick());
         info->setColor(response->getColor());
         info->setKilled(true);

         addPlayerInfo(id, info);

         // send infomap to all instances interested in them
         playerInfoMapUpdatedSignal(&_player_info);

         // play "player joined sample"
         if (!GamePlayback::getInstance()->isReplaying())
            SoundManager::getInstance()->playSoundPlayerJoined();
      }

      // ensure it was us who joined as join game responses are
      // broadcasted to all players
      if (response->getPlayerId() == getPlayerId())
      {
         // store current player info
         setCurrentPlayerInfo(info);

         // store game id
         setGameId(response->getGameId());

         GameInformation* game_information = getGameInformation(getGameId());

         int width = 0;
         int height = 0;
         switch (game_information->getMapDimensions())
         {
            case Constants::Dimension13x11:
               width = 13;
               height = 11;
               break;
            case Constants::Dimension19x17:
               width = 19;
               height = 17;
               break;
            case Constants::Dimension25x21:
               width = 25;
               height = 21;
               break;

            default:
               break;
         }

         playfieldSizeSignal(width, height);
         playfieldScaleSignal(game_information->getMapScaleX(), game_information->getMapScaleY());
         loadLevelSignal(game_information->getLevelName());
         joinGameResponseSignal(true);
      }
   }
   else if (response->getPlayerId() == getPlayerId())
   {
      // the player which did not successfully login was me!
      HelpManager::getInstance()->addMessage("", TEXT_ERROR_UNABLE_TO_JOIN, Constants::HelpSeverityError);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param path path of the level that has been loaded
*/
void BombermanClient::levelLoaded(const std::string& /*path*/)
{
   // game successfully joined
   // emit joinGameResponse(true);

   PlayerSynchronizePacket packet(PlayerSynchronizePacket::LevelLoaded);
   send(&packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::processListGameResponse(Packet* packet)
{
   ListGamesResponsePacket* list = dynamic_cast<ListGamesResponsePacket*>(packet);

   // only update game(s) included in list
   if (list->isUpdate())
   {
      for (const GameInformation& info : list->getGames())
      {
         *(getGameInformation(info.getId())) = info;
      }
   }
   else
   {
      _games = list->getGames();
   }

   // qDebug("BombermanClient::data(): %d games received", _games.size());

   gamesListUpdatedSignal(_games);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processLoginResponse(Packet* packet)
{
   LoginResponsePacket* login = dynamic_cast<LoginResponsePacket*>(packet);

   setPlayerId(login->getId());

   // login is always supposed to work
   playerIdSignal(getPlayerId());
   loginResponseSignal(true);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processPlayerKilled(Packet* packet)
{
   PlayerKilledPacket* kill = dynamic_cast<PlayerKilledPacket*>(packet);

   // inform game drawable about death
   removePlayerSignal(kill->getPlayerId());

   // check if own player was killed
   if (kill->getPlayerId() == getPlayerId())
      rumbleSignal(0.5f, 2000);

   // update player info instance
   PlayerInfo* killed_player = getPlayerInfo(kill->getPlayerId());
   if (killed_player)
   {
      killed_player->setKilled(true);
   }

   // play killed sample
   SoundManager::getInstance()->playSoundKilled();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processPlayerInfected(Packet* packet)
{
   PlayerInfectedPacket* infected_packet = dynamic_cast<PlayerInfectedPacket*>(packet);

   PlayerInfo* player = getPlayerInfo(infected_packet->getPlayerId());

   if (player)
   {
      /*
      // create guarded pointer with disease
      QPointer<PlayerDisease> disease = new PlayerDisease();
      disease.data()->setType(infected_packet->getSkullType());
      disease.data()->setDuration(infected_packet->getDuration());
      disease->activate();

      // infect player
      player->infect(disease);
      */

      playerInfectedSignal(
         infected_packet->getPlayerId(),
         infected_packet->getSkullType(),
         infected_packet->getInfectorId(),
         infected_packet->getExtraPosX(),
         infected_packet->getExtraPosY()
      );

      SoundManager::getInstance()->playSkullSound(infected_packet->getSkullType());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processDetonation(Packet* packet)
{
   DetonationPacket* det = dynamic_cast<DetonationPacket*>(packet);

   // Vector pos = Vector(det->getX(), det->getY(), 0.0);
   //
   // TODO: pos needs to be rotated and scaled properly
   //       right now 0,0 is closest to the listener

   SoundManager::getInstance()->playSoundBomb();

   int intense = std::max<int>(det->getUp(), det->getDown());
   intense = std::max<int>(intense, det->getLeft());
   intense = std::max<int>(intense, det->getRight());

   detonationSignal(det->getX(), det->getY(), det->getUp(), det->getDown(), det->getLeft(), det->getRight(), static_cast<float>(intense));
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processError(Packet* packet)
{
   ErrorPacket* error_packet = dynamic_cast<ErrorPacket*>(packet);

   // show error received from server
   HelpManager::getInstance()->addMessage("", error_packet->getErrorMessage(), Constants::HelpSeverityError);

   switch (error_packet->getErrorType())
   {
      // if sync fails, the player is kicked out of the game
      case Constants::ErrorSyncTimeout:
      {
         resetGameData();

         showMenuSignal();
         showMainMenuSignal();
         break;
      }

      default:
      {
         break;
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
   \return player info ptr
*/
PlayerInfo* BombermanClient::getPlayerInfo(int id) const
{
   PlayerInfo* p_info = nullptr;

   auto iter = _player_info.find(id);

   if (iter != _player_info.end())
      p_info = iter->second;

   return p_info;
}

//-----------------------------------------------------------------------------
/*!
   \param info current player info
*/
void BombermanClient::setCurrentPlayerInfo(PlayerInfo* info)
{
   _current_player_info = info;
}

//-----------------------------------------------------------------------------
/*!
   \return player info ptr
*/
PlayerInfo* BombermanClient::getCurrentPlayerInfo() const
{
   return _current_player_info;
}

//-----------------------------------------------------------------------------
/*!
   \return list of players
*/
std::vector<PlayerInfo*> BombermanClient::getPlayerInfoList() const
{
   std::vector<PlayerInfo*> list;
   list.reserve(_player_info.size());
   for (const auto& [id, info] : _player_info)
      list.push_back(info);
   return list;
}

//-----------------------------------------------------------------------------
/*!
   \return map of players
*/
std::map<int, PlayerInfo*>* BombermanClient::getPlayerInfoMap() const
{
   return &_player_info;
}

//-----------------------------------------------------------------------------
/*!
   \return ptr to position interpolation
*/
PositionInterpolation* BombermanClient::getPositionInterpolation() const
{
   return _position_interpolation.get();
}

//-----------------------------------------------------------------------------
/*!
   \return current message
*/
const std::string& BombermanClient::getMessage() const
{
   return _message;
}

//-----------------------------------------------------------------------------
/*!
   \param message message to set
*/
void BombermanClient::setMessage(const std::string& message)
{
   _message = message;
}

//-----------------------------------------------------------------------------
/*!
   \param connected connected state
*/
void BombermanClient::setConnected(bool connected)
{
   _connected = connected;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if client is connected
*/
bool BombermanClient::isConnected() const
{
   return _connected;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if we're hosting games
*/
bool BombermanClient::isHosting() const
{
   return _server.get();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processPosition(Packet* packet)
{
   PositionPacket* pos_packet = dynamic_cast<PositionPacket*>(packet);

   PlayerInfo* player_info = getPlayerInfo(pos_packet->getPlayerId());

   if (player_info)
   {
      player_info->setPosition(pos_packet->getX(), pos_packet->getY(), pos_packet->getAngle());

      player_info->setPositionDelta(pos_packet->getDeltaX(), pos_packet->getDeltaY(), pos_packet->getAngleDelta());

      player_info->setDirections(pos_packet->getDirections());

      setPlayerPositionSignal(pos_packet->getPlayerId(), pos_packet->getX(), pos_packet->getY(), pos_packet->getAngle());

      setPlayerSpeedSignal(pos_packet->getPlayerId(), pos_packet->getDeltaX(), pos_packet->getDeltaY(), pos_packet->getAngleDelta());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processMapItemCreated(Packet* packet)
{
   MapItem* item = new MapItem(dynamic_cast<MapItemCreatedPacket*>(packet));
   _map_items[item->getUniqueId()] = item;
   createMapItemSignal(item);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processMapItemMove(Packet* packet)
{
   MapItemMovePacket* move_packet = dynamic_cast<MapItemMovePacket*>(packet);
   MapItem* item = getMapItem(move_packet->getMapItemId());

   if (item)
   {
      moveMapItemSignal(item, move_packet->getDirection(), move_packet->getSpeed(), move_packet->getNominalX(), move_packet->getNominalY());

      // play kick sound
      if (move_packet->getSpeed() > 0.0f)
         SoundManager::getInstance()->playSoundKick();
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processExtraMapItemCreated(Packet* packet)
{
   ExtraMapItem* extra = new ExtraMapItem(dynamic_cast<ExtraMapItemCreatedPacket*>(packet));
   _map_items[extra->getUniqueId()] = extra;
   createMapItemSignal(extra);

   SoundManager::getInstance()->playSoundExtraRevealed();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processGameStats(Packet* packet)
{
   GameStatsPacket* stats_packet = dynamic_cast<GameStatsPacket*>(packet);

   // stats are copied
   std::vector<int> ids = stats_packet->getPlayerIds();
   std::vector<PlayerStats> overall_stats = stats_packet->getOverallStats();
   std::vector<PlayerStats> round_stats = stats_packet->getRoundStats();

   PlayerInfo* info = nullptr;

   int i = 0;
   for (int id : ids)
   {
      auto iter = _player_info.find(id);

      if (iter != _player_info.end())
      {
         info = iter->second;

         /*
         qDebug(
            "BombermanClient::processGameStats: "
            "player %s: wins: %d, kills: %d, deaths: %d",
            qPrintable(info->getNick()),
            stats[i].getWins(),
            stats[i].getKills(),
            stats[i].getDeaths()
         );
         */

         info->setOverallStats(overall_stats[i]);
         info->setRoundStats(round_stats[i]);
      }

      i++;
   }

   playerInfoMapUpdatedSignal(&_player_info);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processExtraMapItemDestroyed(Packet* packet)
{
   MapItemDestroyedPacket* remove = dynamic_cast<MapItemDestroyedPacket*>(packet);
   MapItem* item = getMapItem(remove->getUniqueId());

   if (item)
   {
      item->setDestroyDirection(remove->getDirection());

      destroyMapItemSignal(item, remove->getIntensity());
      _map_items.erase(item->getUniqueId());
      delete item;
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processMapItemRemoved(Packet* packet)
{
   MapItemRemovedPacket* remove = dynamic_cast<MapItemRemovedPacket*>(packet);
   MapItem* item = getMapItem(remove->getUniqueId());

   if (item)
   {
      removeMapItemSignal(item);
      _map_items.erase(item->getUniqueId());
      delete item;
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::broadcastAddPlayerData()
{
   for (const auto& [id, player_info] : _player_info)
   {
      // reset killed flag
      player_info->setKilled(false);

      // tell game drawable about players in the game
      addPlayerSignal(player_info->getId(), player_info->getNick(), player_info->getColor());
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::broadcastPlayerStartPositions()
{
   for (const auto& [id, player_info] : _player_info)
   {
      setPlayerPositionSignal(player_info->getId(), player_info->getX(), player_info->getY(), player_info->getAngle());
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::processStartGameResponse(Packet* packet)
{
   StartGameResponsePacket* response = dynamic_cast<StartGameResponsePacket*>(packet);

   qDebug("BombermanClient::data: game %d started", response->getId());

   setIngameMessagingActive(false);
   GameStateMachine::getInstance()->setState(Constants::GameActive);

   _dead = false;

   gameStartedSignal();

   SoundManager* sm = SoundManager::getInstance();
   sm->playSoundStart();
   sm->restartPlayListAfterFadeOut(1000);

   // resend keys pressed on start game
   //	fix for 0000032: Player movement at start of each round
   //    If you press a movement button during the countdown at the start of a
   //    round and hold it, the player does not move when the round begins.
   sendKeysPressedPacket();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processStopGameResponse(Packet* packet)
{
   StopGameResponsePacket* response = dynamic_cast<StopGameResponsePacket*>(packet);

   qDebug("BombermanClient::data: game %d stopped, all rounds finished: %d", response->getId(), response->isFinished());

   GameStateMachine::getInstance()->setState(Constants::GameStopped);

   _dead = true;

   if (response->isFinished())
   {
      gameStoppedSignal();

      SoundManager* sm = SoundManager::getInstance();
      sm->playSoundStart();
      sm->restartPlayListAfterFadeOut(SHOW_WINNER_TIME_SUM);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processGameEvent(Packet* packet)
{
   GameEventPacket* response = dynamic_cast<GameEventPacket*>(packet);

   switch (response->getGameEvent())
   {
      case GameEventPacket::ExtraCollected:
      {
         SoundManager::getInstance()->playSoundExtra();

         extraRemovedSignal(response->getX(), response->getY(), false, response->getExtraType(), response->getPlayerId());

         // rumble just a little on extra collect
         if (response->getPlayerId() == getPlayerId())
            rumbleSignal(0.2f, 500);

         break;
      }

      case GameEventPacket::ExtraDestroyed:
      {
         extraRemovedSignal(response->getX(), response->getY(), true, response->getExtraType(), -1);

         break;
      }

      case GameEventPacket::BombExploded:
      default:
         break;
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processMessage(Packet* packet)
{
   MessagePacket* message_packet = dynamic_cast<MessagePacket*>(packet);

   messageReceivedSignal(message_packet->getSenderId(), message_packet->getMessage(), message_packet->isTypingFinished());

   if (message_packet->isTypingFinished())
   {
      // omit broadcast messages
      if (message_packet->getSenderId() != -1 && message_packet->getSenderId() != getPlayerId())
      {
         SoundManager::getInstance()->playSoundMessageReceived();
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processTime(Packet* packet)
{
   TimePacket* time_packet = dynamic_cast<TimePacket*>(packet);

   /*
      qDebug(
         "BombermanClient::data: server time: %s, left: %d",
         qPrintable(packet->getTimestamp().toString()),
         time_packet->getTimeLeft()
      );
   */

   GameInformation* game_information = getCurrentGameInformation();

   if (game_information)
   {
      int time_left = time_packet->getTimeLeft();
      int duration = game_information->getDuration();

      if (time_left == (duration * 3 / 20))
      {
         SoundManager::getInstance()->playSoundHurryUp();
      }

      timeChangedSignal(time_left, duration);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processCountdown(Packet* packet)
{
   GameStateMachine::getInstance()->setState(Constants::GamePreparing);

   CountdownPacket* countdown_packet = dynamic_cast<CountdownPacket*>(packet);

   int time_left = countdown_packet->getTimeLeft();

   qDebug("BombermanClient::processCountdown: left: %d", time_left);

   // sync tick
   if (time_left == SERVER_PREPARATION_TIME + SERVER_PREPARATION_SYNC_TIME - 1)
   {
      // use first tick to mute the music while the
      // countdown samples are played
      SoundManager::getInstance()->fadeOut(1000);

      showGameSignal();

      broadcastAddPlayerData();
      broadcastPlayerStartPositions();
   }

   // next ticks
   if (time_left <= SERVER_PREPARATION_TIME - 1)
   {
      countdownSignal(time_left);

      // play sample
      SoundManager::getInstance()->playSoundTime(time_left);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processPacket(Packet* packet)
{
   if (packet)
   {
      switch (packet->getType())
      {
         case Packet::COUNTDOWN:
         {
            processCountdown(packet);
            break;
         }

         case Packet::CREATEGAMERESPONSE:
         {
            processCreateGameResponse(packet);
            break;
         }

         case Packet::DETONATION:
         {
            processDetonation(packet);
            break;
         }

         case Packet::ERROR:
         {
            processError(packet);
            break;
         }

         case Packet::EXTRAMAPITEMCREATED:
         {
            processExtraMapItemCreated(packet);
            break;
         }

         case Packet::GAMESTATS:
         {
            processGameStats(packet);
            break;
         }

         case Packet::GAMEEVENT:
         {
            processGameEvent(packet);
            break;
         }

         case Packet::JOINGAMERESPONSE:
         {
            processJoinGameResponse(packet);
            break;
         }

         case Packet::KEY:
         {
            qWarning("BombermanClient::data(): wrong direction!");
            break;
         }

         case Packet::LISTGAMESRESPONSE:
         {
            processListGameResponse(packet);
            break;
         }

         case Packet::LOGINRESPONSE:
         {
            processLoginResponse(packet);
            break;
         }

         case Packet::PLAYERINFECTEDPACKET:
         {
            processPlayerInfected(packet);
            break;
         }

         case Packet::PLAYERKILLED:
         {
            processPlayerKilled(packet);
            break;
         }

         case Packet::POSITION:
         {
            /*
            qDebug(
               "pos received: %s",
               qPrintable(QTime::currentTime().toString("hh:mm:ss.zzz"))
            );
            */

            processPosition(packet);
            break;
         }

         case Packet::LEAVEGAMERESPONSE:
         {
            processLeaveGameResponse(packet);
            break;
         }

         case Packet::MAPITEMCREATED:
         {
            processMapItemCreated(packet);
            break;
         }

         case Packet::MAPITEMDESTROYED:
         {
            processExtraMapItemDestroyed(packet);
            break;
         }

         case Packet::MAPITEMMOVE:
         {
            processMapItemMove(packet);
            break;
         }

         case Packet::MAPITEMREMOVED:
         {
            processMapItemRemoved(packet);
            break;
         }

         case Packet::MESSAGE:
         {
            processMessage(packet);
            break;
         }

         case Packet::STARTGAMERESPONSE:
         {
            processStartGameResponse(packet);
            break;
         }

         case Packet::STOPGAMERESPONSE:
         {
            processStopGameResponse(packet);
            break;
         }

         case Packet::TIME:
         {
            processTime(packet);
            break;
         }

         case Packet::EXTRASHAKE:
         {
            processExtraShake(packet);
            break;
         }

         case Packet::INVALID:
         {
            qWarning("BombermanClient::data: Packet::INVALID received");
            break;
         }

         default:
            break;
      }

      // if playback is activated, record packets - ownership stays with the caller's
      // unique_ptr<Packet> either way, record() only observes it
      if (GamePlayback::getInstance()->isRecording())
      {
         GamePlayback::getInstance()->record(packet);
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::readData()
{
   char chunk[4096];
   int bytes_read;

   while ((bytes_read = NET_ReadFromStreamSocket(_socket, chunk, sizeof(chunk))) > 0)
   {
      _buffer.append(chunk, bytes_read);
   }

   if (bytes_read < 0)
   {
      NET_DestroyStreamSocket(_socket);
      _socket = nullptr;
      clientDisconnect();
      return;
   }

   while (packetAvailable())
   {
      // block was read completely
      BinaryReader in = _buffer.reader();
      auto packet = Packet::deserialize(in);
      _buffer.consume(in.pos());

      processPacket(packet.get());

      _block_size = 0;
   }

   _buffer.compact();
}

//-----------------------------------------------------------------------------
/*!
   poll for connection progress and incoming data, once per tick
*/
void BombermanClient::poll()
{
   if (_address)
   {
      NET_Status status = NET_GetAddressStatus(_address);

      if (status == NET_SUCCESS)
      {
         NET_Address* address = _address;
         _address = nullptr;

         _socket = NET_CreateClient(address, 6300, 0);
         NET_UnrefAddress(address);

         if (!_socket)
         {
            reportConnectionError(SDL_GetError());
         }
      }
      else if (status == NET_FAILURE)
      {
         reportConnectionError(SDL_GetError());
         NET_UnrefAddress(_address);
         _address = nullptr;
      }

      return;
   }

   if (_socket && !isConnected())
   {
      NET_Status status = NET_GetConnectionStatus(_socket);

      if (status == NET_SUCCESS)
      {
         clientConnect();
         connectedSignal();
      }
      else if (status == NET_FAILURE)
      {
         reportConnectionError(SDL_GetError());
         NET_DestroyStreamSocket(_socket);
         _socket = nullptr;
      }

      return;
   }

   if (_socket && isConnected())
   {
      readData();
   }
}

//-----------------------------------------------------------------------------
/*!
   \param event keypressed event
*/
void BombermanClient::keyPressed(const KeyEvent& event)
{
   GameSettings::ControllerSettings* controller_settings = GameSettings::getInstance()->getControllerSettings();

   bool control_key = event.key() == controller_settings->getUpKey() || event.key() == controller_settings->getDownKey() ||
                     event.key() == controller_settings->getLeftKey() || event.key() == controller_settings->getRightKey() ||
                     event.key() == controller_settings->getBombKey() || event.key() == controller_settings->getZoomInKey() ||
                     event.key() == controller_settings->getZoomOutKey() || event.key() == controller_settings->getStartKey();

   if ((control_key && !event.isAutoRepeat()) || !control_key)
   {
      processKeyPressed(event.key());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param event keyreleased event
*/
void BombermanClient::keyReleased(const KeyEvent& event)
{
   if (!event.isAutoRepeat())
      processKeyReleased(event.key());
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::releaseAllKeys()
{
   if (_keys_pressed != 0)
   {
      // next bomb may be dropped
      _bomb_released = true;
      _keys_pressed = 0;

      // send key changes
      processKeyReleased(0);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::clearPlayerInfoMap()
{
   for (const auto& [id, info] : _player_info)
   {
      if (info == _current_player_info)
         _current_player_info = nullptr;

      delete info;
   }
   _player_info.clear();
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
*/
void BombermanClient::removePlayerInfo(int id)
{
   auto it = _player_info.find(id);
   if (it != _player_info.end())
   {
      if (it->second == _current_player_info)
         _current_player_info = nullptr;

      delete it->second;
      _player_info.erase(it);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processLeaveGameResponse(Packet* packet)
{
   LeaveGameResponsePacket* leave_packet = dynamic_cast<LeaveGameResponsePacket*>(packet);

   // either we left the game, so we have to clear the player info map
   // completely, or it was just one player, so the player map keeps intact
   if (leave_packet->getPlayerId() == getPlayerId())
   {
      clearPlayerInfoMap();
   }
   else
   {
      // remove player from playerinfo-map
      removePlayerInfo(leave_packet->getPlayerId());
   }

   // send infomap to all instances interested in them
   playerInfoMapUpdatedSignal(&_player_info);

   // remove player
   removePlayerSignal(leave_packet->getPlayerId());

   // play "player left sample"
   if (!GamePlayback::getInstance()->isReplaying())
      SoundManager::getInstance()->playSoundPlayerLeft();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processExtraShake(Packet* packet)
{
   ExtraShakePacket* shake_packet = dynamic_cast<ExtraShakePacket*>(packet);

   MapItem* item = getMapItem(shake_packet->getMapItemUniqueId());

   if (item)
   {
      shakeBlockSignal(item);
      SoundManager::getInstance()->playSoundBoxShake();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::sendKeysPressedPacket()
{
   // build keypacket
   // qDebug("BombermanClient::keyPressed: sending keypacket");
   KeyPacket kPacket(0, _keys_pressed);
   send(&kPacket);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::removeBombKeyFlag()
{
   // in any case remove the bomb key from the key combination
   // to avoid unwanted bomb dropping
   if ((_keys_pressed & Constants::KeyBomb) == Constants::KeyBomb)
      _keys_pressed &= ~Constants::KeyBomb;
}

//-----------------------------------------------------------------------------
/*!
   \param key key to process
   \param text text to type
*/
void BombermanClient::processKeyPressed(int key)
{
   if (key == SDLK_RETURN || key == SDLK_KP_ENTER)
   {
      if (GameStateMachine::getInstance()->getState() == Constants::GameActive)
      {
         toggleIngameMessaging();
      }
   }

   // write text
   if (isIngameMessagingActive())
   {
      if (GameStateMachine::getInstance()->getState() == Constants::GameActive)
      {
         if (key == SDLK_ESCAPE)
         {
            toggleIngameMessaging();
         }
      }
   }

   // leave game if requested
   else if (key == SDLK_ESCAPE)
   {
      // restart music if escape was hit during countdown
      if (GameStateMachine::getInstance()->getState() == Constants::GamePreparing)
      {
         SoundManager* sm = SoundManager::getInstance();
         sm->restartPlayListAfterFadeOut(1000);
      }

      leaveGameRequest();
      _bot_factory->removeAll();

      // reset game data is called before showMenu/showMainMenu
      // as the win drawable sets itself visible when the game state
      // is changed to 'stopped' (that happens in resetGameData)
      resetGameData();

      showMenuSignal();
      showMainMenuSignal();
   }

   // zoom camera
   else if (key == SDLK_LEFTBRACKET)
   {
      zoomOutSignal(true);
   }
   else if (key == SDLK_RIGHTBRACKET)
   {
      zoomInSignal(true);
   }

   // abort game (F10/Start)
   else if (key == SDLK_F10)
   {
      stopGame();
   }

   // care about movement
   else
   {
      bool moved = false;

      GameSettings::ControllerSettings* controller_settings = GameSettings::getInstance()->getControllerSettings();

      if (key == controller_settings->getUpKey())
      {
         _keys_pressed |= Constants::KeyUp;
         moved = true;
      }
      else if (key == controller_settings->getDownKey())
      {
         _keys_pressed |= Constants::KeyDown;
         moved = true;
      }
      else if (key == controller_settings->getLeftKey())
      {
         _keys_pressed |= Constants::KeyLeft;
         moved = true;
      }
      else if (key == controller_settings->getRightKey())
      {
         _keys_pressed |= Constants::KeyRight;
         moved = true;
      }
      else if (key == controller_settings->getBombKey()
               // || key == Qt::Key_Space
               // || key == Qt::Key_Control
      )
      {
         // TODO: verify if omitting the bombrelease flag is alright
         //       in the past keyrelease events were dropped and the
         //       flag was not reset properly, therefore bombkey-presses
         //       were left out.. duh.
         //
         if (true /*_bomb_released*/)
         {
            // in any case remove the bomb key in order to avoid dropping
            // too many bombs (i.e. autofire is disabled for bombs)
            _bomb_released = false;
            _keys_pressed |= Constants::KeyBomb;
            moved = true;
         }
      }

      if (moved)
      {
         if (!_dead)
         {
            sendKeysPressedPacket();
            removeBombKeyFlag();
         }
      }
   }
}

void BombermanClient::processKeyReleased(int key)
{
   GameSettings::ControllerSettings* controller_settings = GameSettings::getInstance()->getControllerSettings();

   if (key == controller_settings->getUpKey())
   {
      _keys_pressed &= ~Constants::KeyUp;
   }
   else if (key == controller_settings->getDownKey())
   {
      _keys_pressed &= ~Constants::KeyDown;
   }
   else if (key == controller_settings->getLeftKey())
   {
      _keys_pressed &= ~Constants::KeyLeft;
   }
   else if (key == controller_settings->getRightKey())
   {
      _keys_pressed &= ~Constants::KeyRight;
   }
   // zoom camera
   else if (key == SDLK_LEFTBRACKET)
   {
      zoomOutSignal(false);
   }
   else if (key == SDLK_RIGHTBRACKET)
   {
      zoomInSignal(false);
   }
   else if (key == controller_settings->getBombKey())
   {
      // next bomb may be dropped
      _bomb_released = true;
      _keys_pressed &= ~Constants::KeyBomb;
   }

   /*
      switch (key)
      {
         case Qt::Key_Up:
            _keys_pressed &= ~Constants::KeyUp;
            break;

         case Qt::Key_Down:
            _keys_pressed &= ~Constants::KeyDown;
            break;

         case Qt::Key_Left:
            _keys_pressed &= ~Constants::KeyLeft;
            break;

         case Qt::Key_Right:
            _keys_pressed &= ~Constants::KeyRight;
            break;

         case Qt::Key_Control:
         {
            // next bomb may be dropped
            _bomb_released = true;
            _keys_pressed &= ~Constants::KeyBomb;
         }

         default:
            break;
      }
   */

   if (!_dead)
   {
      // build keypacket
      // qDebug("BombermanClient::keyPressed: sending keypacket");
      KeyPacket kPacket(0, _keys_pressed);
      send(&kPacket);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::debugKeyboardInput()
{
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::resetClientState()
{
   setConnected(false);
   _dead = true;

   // we're disconnected
   loginResponseSignal(false);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::resetGameData()
{
   for (const auto& [id, p] : _player_info)
   {
      removePlayerSignal(p->getId());
   }

   clearPlayerInfoMap();
   _games.clear();

   setGameId(-1);

   GameStateMachine::getInstance()->setState(Constants::GameStopped);
}

/*
   server workflow

   1) LoginRequest, wait for
      LoginResponse

   2) ListGameRequest, wait for
      ListGameResponse

   3) CreateGameRequest if required, wait for
      CreateGameResponse

   4) JoinGameRequest, wait for
      JoinGameResponse (for each player in the game)

   5) StartGameRequest, wait for
      StartGameResponse

*/

//-----------------------------------------------------------------------------
/*!
   \param nick nick to use
   \param color color to use
*/
void BombermanClient::login(const std::string& nick)
{
   setNick(nick);
   LoginRequestPacket login(nick, false);
   send(&login);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::stopGame()
{
   stopGame(getGameId());
}

//-----------------------------------------------------------------------------
/*!
   \param player's nick
*/
void BombermanClient::setNick(const std::string& nick)
{
   _nick = nick;
}

//-----------------------------------------------------------------------------
/*!
   \return player's nick
*/
const std::string& BombermanClient::getNick() const
{
   return _nick;
}

//-----------------------------------------------------------------------------
/*!
   \param message message to send
   \param receiverId id of the message receiver
*/
void BombermanClient::sendMessage(const std::string& message, bool finishedTyping, int receiverId)
{
   if (!finishedTyping)
      setMessage(message);

   MessagePacket packet(-1, message, finishedTyping, receiverId);

   send(&packet);

   if (finishedTyping)
   {
      setMessage("");
      SoundManager::getInstance()->playSoundMessageSent();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::listGames()
{
   if (isConnected() && (GameStateMachine::getInstance()->getState() != Constants::GameActive))
   {
      ListGamesRequestPacket packet;
      send(&packet);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::createGameAutomatic()
{
   createGame(
      "coding", Level::getLevelDirectoryName(Level::LevelCastle), 1, 1800, 10, true, true, true, true, true, Constants::Dimension13x11
   );
}

//-----------------------------------------------------------------------------
/*!
   \param name game's name
*/
void BombermanClient::createGame(
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
)
{
   bool dry_run = GameSettings::getInstance()->getDevelopmentSettings()->isDryRunEnabled();

   if (dry_run)
      rounds = 999;

   CreateGameRequestPacket packet(
      name,
      level,
      rounds,
      duration,
      maxPlayers,
      extraBombEnabled,
      extraFlameEnabled,
      extraSpeedupEnabled,
      extraKickEnabled,
      extraSkullsEnabled,
      dimension
   );

   send(&packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::joinGame(int game)
{
   JoinGameRequestPacket packet(game);
   send(&packet);
}

//-----------------------------------------------------------------------------
/*!
   \param game game to start
*/
void BombermanClient::startGame(int game)
{
   GameStateMachine::getInstance()->setState(Constants::GamePreparing);

   StartGameRequestPacket packet(game);
   send(&packet);
}

//-----------------------------------------------------------------------------
/*!
   \param game game to stop
*/
void BombermanClient::stopGame(int game)
{
   StopGameRequestPacket request(game);
   send(&request);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::setLoginAfterConnect(bool login_after_connect)
{
   _login_after_connect = login_after_connect;
}

//-----------------------------------------------------------------------------
/*!
   \param host host to use
   \param nick nick to use
*/
void BombermanClient::loginRequest(const std::string& host, const std::string& nick)
{
   const std::string previous_host = getHost();
   const bool connected_or_connecting = (_socket != nullptr) || (_address != nullptr);

   setHost(host);
   setNick(nick);

   // we first have to disconnect from the current server
   // because its hostname obviously differs from ours
   if (connected_or_connecting && StringUtils::toLower(previous_host) != StringUtils::toLower(host))
   {
      qDebug(
         "BombermanClient::loginRequest: connect to another host: %s -> %s",
         StringUtils::toLower(previous_host).c_str(),
         StringUtils::toLower(host).c_str()
      );

      setConnected(false);
      disconnectFromServer();
   }
   else
   {
      if (!isConnected())
      {
         setConnected(false);
         connectToServer();
         _login_after_connect = true;
      }
      else
      {
         // we are already connected, so we just need to login
         login(nick);
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::leaveGameRequest()
{
   LeaveGameRequestPacket packet(getGameId(), getPlayerId());
   send(&packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::gameListRequest()
{
   listGames();
}

//----------------------------------------------------------------------------
/*!
 */
void BombermanClient::host()
{
   if (!_server)
   {
      _server = std::make_unique<Server>();

      if (_server->isListening())
      {
         // runs on its own thread, decoupled from the client's render/frame rate
         _server_thread = std::jthread(
            [this](std::stop_token stop_token)
            {
               _server->startPolling();

               while (!stop_token.stop_requested())
               {
                  Timer::update();

                  // std::this_thread::sleep_for() is quantized to the OS scheduler's timer
                  // resolution (~15.6ms on Windows regardless of what's requested) - spin-waiting
                  // via yield() isn't, so it replaces the sleep here.
                  auto next = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
                  while (std::chrono::steady_clock::now() < next && !stop_token.stop_requested())
                  {
                     std::this_thread::yield();
                  }
               }
            }
         );

         hostingSignal(true);
      }
      else
      {
         HelpManager::getInstance()->addMessage("", TEXT_ERROR_UNABLE_TO_BIND, Constants::HelpSeverityError);

         _server.reset();
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::initializeBots()
{
   if (isPlayerOwner())
   {
      // pass relevant data to bot factory
      _bot_factory->setHostname(getHost());
      _bot_factory->setGameId(getGameId());

      int bots = 0;

      if (isSinglePlayer())
         bots = GameSettings::getInstance()->getCreateGameSettingsSingle()->getBotCount();
      else
         bots = GameSettings::getInstance()->getCreateGameSettingsMulti()->getBotCount();

      _bot_factory->add(bots);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::initializePlayback()
{
   bool recording = GameSettings::getInstance()->getDevelopmentSettings()->isGameRecordingEnabled();

   GamePlayback::getInstance()->setRecording(recording);
}

//-----------------------------------------------------------------------------
/*!
   \return local ips
*/
std::vector<std::string> BombermanClient::getLocalIps() const
{
   std::vector<std::string> ips;

   int count = 0;
   NET_Address** addresses = NET_GetLocalAddresses(&count);

   if (addresses)
   {
      for (int i = 0; i < count; i++)
      {
         const char* address = NET_GetAddressString(addresses[i]);

         if (!address)
            continue;

         std::string ip(address);

         // IPv4 only, no loopback
         if (ip.find(':') == std::string::npos && ip != "127.0.0.1")
            ips.push_back(ip);
      }

      NET_FreeLocalAddresses(addresses);
   }

   return ips;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if game is single player
*/
bool BombermanClient::isSinglePlayer() const
{
   bool single_player = false;

   single_player = (getGameMode() == Constants::GameModeSinglePlayer);

   return single_player;
}

//-----------------------------------------------------------------------------
/*!
   \return game mode
*/
Constants::GameMode BombermanClient::getGameMode() const
{
   return _game_mode;
}

//-----------------------------------------------------------------------------
/*!
   \param mode game mode
*/
void BombermanClient::setGameMode(const Constants::GameMode& mode)
{
   _game_mode = mode;
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::playbackFinished()
{
   leaveGameRequest();
   showMenuSignal();
   showMainMenuSignal();
   resetGameData();
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::gameStateChanged()
{
   if (GameStateMachine::getInstance()->getState() == Constants::GameStopped)
   {
      setIngameMessagingActive(false);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param idle idle mode flag
*/
void BombermanClient::idle(bool idle)
{
   if (idle)
   {
      if (GameStateMachine::getInstance()->getState() == Constants::GameStopped)
      {
         // check if we're in main menu right now
         if (isMainMenuActive())
         {
            GamePlayback::getInstance()->playDemo();
         }
      }
   }
   else
   {
      if (GamePlayback::getInstance()->isReplaying())
      {
         GamePlayback::getInstance()->abort();
         playbackFinished();
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param active when main menu is active
*/
void BombermanClient::setMainMenuActive(bool active)
{
   _main_menu_active = active;
}

//-----------------------------------------------------------------------------
/*!
   \param pageName name of the current page
*/
void BombermanClient::showIps()
{
   // only do this when we're somewhere in the menus
   if (GameStateMachine::getInstance()->getState() == Constants::GameStopped)
   {
      std::vector<std::string> ip_list = getLocalIps();

      std::size_t index = 0;
      while (index < ip_list.size())
      {
         // combine two ips per message
         std::string combined = ip_list[index++];
         if (index < ip_list.size())
            combined += ";" + ip_list[index++];

         std::string ip_text = std::format("your ips are;{}", combined);
         HelpManager::getInstance()->addMessage("", ip_text, Constants::HelpSeverityNotification);
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if main menu is shown
*/
bool BombermanClient::isMainMenuActive() const
{
   return _main_menu_active;
}
