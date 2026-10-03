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

#include <array>
#include <chrono>
#include <format>
#include <span>
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
std::optional<std::reference_wrapper<BombermanClient>> BombermanClient::_instance;

//-----------------------------------------------------------------------------
/*!
 */
BombermanClient::BombermanClient(/*const std::string& host, const std::string& nick*/)
{
   _instance = *this;

   _position_interpolation = std::make_unique<PositionInterpolation>();

   _position_interpolation->bounceSignal.connect([]() { SoundManager::getInstance().playSoundBombBounce(); });

   GameStateMachine::getInstance().stateChangedSignal.connect([this]() { gameStateChanged(); });

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
   moveMapItemSignal.connect([this](const MapItem& item, Constants::Direction dir, float speed, int nominalX, int nominalY)
                             { _position_interpolation->moveMapItem(item, dir, speed, nominalX, nominalY); });
}

//-----------------------------------------------------------------------------
/*!
 */
BombermanClient::~BombermanClient()
{
   _instance.reset();
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
BombermanClient& BombermanClient::getInstance()
{
   return _instance.value();
}

//-----------------------------------------------------------------------------
/*!
   \return \c true while a client exists
*/
bool BombermanClient::hasInstance()
{
   return _instance.has_value();
}

//-----------------------------------------------------------------------------
/*!
   \return list of games
*/
std::vector<GameInformation>& BombermanClient::getGames()
{
   return _games;
}

//-----------------------------------------------------------------------------
/*!
   \return list of games
*/
const std::vector<GameInformation>& BombermanClient::getGames() const
{
   return _games;
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
std::optional<std::reference_wrapper<GameInformation>> BombermanClient::getGameInformation(int id)
{
   const auto game = std::ranges::find_if(_games, [id](const GameInformation& info) { return info.getId() == id; });
   if (game == _games.end())
   {
      return std::nullopt;
   }
   return *game;
}

//-----------------------------------------------------------------------------
/*!
   \return game information
*/
std::optional<std::reference_wrapper<const GameInformation>> BombermanClient::getGameInformation(int id) const
{
   const auto game = std::ranges::find_if(_games, [id](const GameInformation& info) { return info.getId() == id; });
   if (game == _games.end())
   {
      return std::nullopt;
   }
   return *game;
}

//-----------------------------------------------------------------------------
/*!
   \return game information
*/
std::optional<std::reference_wrapper<const GameInformation>> BombermanClient::getCurrentGameInformation() const
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
   const auto info = getCurrentGameInformation();

   if (info)
   {
      owner = info->get().getCreatorId() == getPlayerId();
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

   const auto info = getPlayerInfo(player_id);

   if (info)
   {
      color = info->get().getColor();
   }

   return color;
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to send
*/
void BombermanClient::send(Packet& packet)
{
   packet.serialize();

   if (_socket)
   {
      NET_WriteToStreamSocket(_socket.get(), packet.data(), static_cast<int>(packet.size()));
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
   _address.reset(NET_ResolveHostname(getHost().c_str()));

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

   clearMapItems();
}

//-----------------------------------------------------------------------------
/*!
   the game drawable and the position interpolation drop what they know about the items
*/
void BombermanClient::clearMapItems()
{
   while (!_map_items.empty())
   {
      const auto it = _map_items.begin();
      const auto item = std::move(it->second);
      _map_items.erase(it);
      removeMapItemSignal(*item);
   }
}

//-----------------------------------------------------------------------------
/*!
   tear down whatever connection attempt or connection is in progress
*/
void BombermanClient::disconnectFromServer()
{
   _address.reset();
   _socket.reset();

   clientDisconnect();
}

//-----------------------------------------------------------------------------
/*!
  \param reason low-level failure reason, from SDL_GetError()
*/
void BombermanClient::reportConnectionError(std::string_view reason)
{
   std::string message = TEXT_ERROR_NETWORK_GENERAL;

   if (!reason.empty())
   {
      message += std::format(" ({})", reason);
   }

   HelpManager::getInstance().addMessage("", message, Constants::HelpSeverityError);
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
std::optional<std::reference_wrapper<MapItem>> BombermanClient::getMapItem(int id) const
{
   const auto it = _map_items.find(id);
   if (it == _map_items.end())
   {
      return std::nullopt;
   }
   return *it->second;
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processCreateGameResponse(const CreateGameResponsePacket& response)
{
   // extract game information
   GameInformation game_information = response.getGameInformation();
   int game_id = game_information.getId();
   int player_id = game_information.getCreatorId();
   bool success = false;

   if (game_id > -1)
   {
      qDebug("BombermanClient::data(): game %d created", game_id);

      // update game information
      if (const auto old_game_information = getGameInformation(game_id))
      {
         old_game_information->get() = game_information;
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
PlayerInfo& BombermanClient::addPlayerInfo(int id)
{
   _player_info.erase(id);
   return _player_info[id];
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processJoinGameResponse(const JoinGameResponsePacket& response)
{
   if (response.isSuccessful())
   {
      std::optional<int> added_id;

      int id = response.getPlayerId();

      if (!_player_info.contains(id))
      {
         PlayerInfo& info = addPlayerInfo(id);
         info.setId(id);
         info.setNick(response.getNick());
         info.setColor(response.getColor());
         info.setKilled(true);
         added_id = id;

         // send infomap to all instances interested in them
         playerInfoMapUpdatedSignal(_player_info);

         // play "player joined sample"
         if (!GamePlayback::getInstance().isReplaying())
            SoundManager::getInstance().playSoundPlayerJoined();
      }

      // ensure it was us who joined as join game responses are
      // broadcasted to all players
      if (response.getPlayerId() == getPlayerId())
      {
         // store current player info
         setCurrentPlayerId(added_id);

         // store game id
         setGameId(response.getGameId());

         const GameInformation& game_information = getGameInformation(getGameId()).value();

         int width = 0;
         int height = 0;
         switch (game_information.getMapDimensions())
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
         playfieldScaleSignal(game_information.getMapScaleX(), game_information.getMapScaleY());
         loadLevelSignal(game_information.getLevelName());
         joinGameResponseSignal(true);
      }
   }
   else if (response.getPlayerId() == getPlayerId())
   {
      // the player which did not successfully login was me!
      HelpManager::getInstance().addMessage("", TEXT_ERROR_UNABLE_TO_JOIN, Constants::HelpSeverityError);
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
   send(packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::processListGameResponse(const ListGamesResponsePacket& list)
{
   // only update game(s) included in list
   if (list.isUpdate())
   {
      for (const GameInformation& info : list.getGames())
      {
         getGameInformation(info.getId()).value().get() = info;
      }
   }
   else
   {
      _games = list.getGames();
   }

   // qDebug("BombermanClient::data(): %d games received", _games.size());

   gamesListUpdatedSignal(_games);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processLoginResponse(const LoginResponsePacket& login)
{
   setPlayerId(login.getId());

   if (_renaming)
   {
      _renaming = false;
      return;
   }

   // login is always supposed to work
   playerIdSignal(getPlayerId());
   loginResponseSignal(true);
}

//-----------------------------------------------------------------------------
/*!
   \param nick new nick, the server keeps the player's id
*/
void BombermanClient::rename(const std::string& nick)
{
   if (nick.empty() || nick == getNick())
   {
      return;
   }

   setNick(nick);
   _renaming = true;
   LoginRequestPacket login(nick, false);
   send(login);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processPlayerKilled(const PlayerKilledPacket& kill)
{
   // inform game drawable about death
   removePlayerSignal(kill.getPlayerId());

   // check if own player was killed
   if (kill.getPlayerId() == getPlayerId())
      rumbleSignal(0.5f, 2000);

   // update player info instance
   if (const auto killed_player = getPlayerInfo(kill.getPlayerId()))
   {
      killed_player->get().setKilled(true);
   }

   // play killed sample
   SoundManager::getInstance().playSoundKilled();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processPlayerInfected(const PlayerInfectedPacket& infected_packet)
{
   if (getPlayerInfo(infected_packet.getPlayerId()))
   {
      playerInfectedSignal(
         infected_packet.getPlayerId(),
         infected_packet.getSkullType(),
         infected_packet.getInfectorId(),
         infected_packet.getExtraPosX(),
         infected_packet.getExtraPosY()
      );

      SoundManager::getInstance().playSkullSound(infected_packet.getSkullType());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processDetonation(const DetonationPacket& det)
{
   // Vector pos = Vector(det.getX(), det.getY(), 0.0);
   //
   // TODO: pos needs to be rotated and scaled properly
   //       right now 0,0 is closest to the listener

   SoundManager::getInstance().playSoundBomb();

   int intense = std::max<int>(det.getUp(), det.getDown());
   intense = std::max<int>(intense, det.getLeft());
   intense = std::max<int>(intense, det.getRight());

   detonationSignal(det.getX(), det.getY(), det.getUp(), det.getDown(), det.getLeft(), det.getRight(), static_cast<float>(intense));
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processError(const ErrorPacket& error_packet)
{
   // show error received from server
   HelpManager::getInstance().addMessage("", error_packet.getErrorMessage(), Constants::HelpSeverityError);

   switch (error_packet.getErrorType())
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
std::optional<std::reference_wrapper<PlayerInfo>> BombermanClient::getPlayerInfo(int id)
{
   const auto iter = _player_info.find(id);
   if (iter == _player_info.end())
   {
      return std::nullopt;
   }
   return iter->second;
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
   \return player info
*/
std::optional<std::reference_wrapper<const PlayerInfo>> BombermanClient::getPlayerInfo(int id) const
{
   const auto iter = _player_info.find(id);
   if (iter == _player_info.end())
   {
      return std::nullopt;
   }
   return iter->second;
}

//-----------------------------------------------------------------------------
/*!
   \param id id of the current player info, none if there is no current player
*/
void BombermanClient::setCurrentPlayerId(std::optional<int> id)
{
   _current_player_id = id;
}

//-----------------------------------------------------------------------------
/*!
   \return current player info
*/
std::optional<std::reference_wrapper<const PlayerInfo>> BombermanClient::getCurrentPlayerInfo() const
{
   if (!_current_player_id)
   {
      return std::nullopt;
   }
   return getPlayerInfo(*_current_player_id);
}

//-----------------------------------------------------------------------------
/*!
   \return list of players
*/
std::vector<std::reference_wrapper<const PlayerInfo>> BombermanClient::getPlayerInfoList() const
{
   std::vector<std::reference_wrapper<const PlayerInfo>> list;
   list.reserve(_player_info.size());
   for (const auto& [id, info] : _player_info)
   {
      list.emplace_back(info);
   }
   return list;
}

//-----------------------------------------------------------------------------
/*!
   \return map of players
*/
std::map<int, PlayerInfo>& BombermanClient::getPlayerInfoMap()
{
   return _player_info;
}

//-----------------------------------------------------------------------------
/*!
   \return map of players
*/
const std::map<int, PlayerInfo>& BombermanClient::getPlayerInfoMap() const
{
   return _player_info;
}

//-----------------------------------------------------------------------------
/*!
   \return position interpolation
*/
PositionInterpolation& BombermanClient::getPositionInterpolation() const
{
   return *_position_interpolation;
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
void BombermanClient::processPosition(const PositionPacket& pos_packet)
{
   if (const auto info = getPlayerInfo(pos_packet.getPlayerId()))
   {
      PlayerInfo& player_info = *info;

      player_info.setPosition(pos_packet.getX(), pos_packet.getY(), pos_packet.getAngle());

      player_info.setPositionDelta(pos_packet.getDeltaX(), pos_packet.getDeltaY(), pos_packet.getAngleDelta());

      player_info.setDirections(pos_packet.getDirections());

      setPlayerPositionSignal(pos_packet.getPlayerId(), pos_packet.getX(), pos_packet.getY(), pos_packet.getAngle());

      setPlayerSpeedSignal(pos_packet.getPlayerId(), pos_packet.getDeltaX(), pos_packet.getDeltaY(), pos_packet.getAngleDelta());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processMapItemCreated(const MapItemCreatedPacket& packet)
{
   auto item = std::make_unique<MapItem>(packet);
   const MapItem& created = *item;
   _map_items[created.getUniqueId()] = std::move(item);
   createMapItemSignal(created);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processMapItemMove(const MapItemMovePacket& move_packet)
{
   if (const auto item = getMapItem(move_packet.getMapItemId()))
   {
      moveMapItemSignal(*item, move_packet.getDirection(), move_packet.getSpeed(), move_packet.getNominalX(), move_packet.getNominalY());

      // play kick sound
      if (move_packet.getSpeed() > 0.0f)
         SoundManager::getInstance().playSoundKick();
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processExtraMapItemCreated(const ExtraMapItemCreatedPacket& packet)
{
   auto extra = std::make_unique<ExtraMapItem>(packet);
   const MapItem& created = *extra;
   _map_items[created.getUniqueId()] = std::move(extra);
   createMapItemSignal(created);

   SoundManager::getInstance().playSoundExtraRevealed();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processGameStats(const GameStatsPacket& stats_packet)
{
   // stats are copied
   std::vector<int> ids = stats_packet.getPlayerIds();
   std::vector<PlayerStats> overall_stats = stats_packet.getOverallStats();
   std::vector<PlayerStats> round_stats = stats_packet.getRoundStats();

   int i = 0;
   for (int id : ids)
   {
      auto iter = _player_info.find(id);

      if (iter != _player_info.end())
      {
         PlayerInfo& info = iter->second;

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

         info.setOverallStats(overall_stats[i]);
         info.setRoundStats(round_stats[i]);
      }

      i++;
   }

   playerInfoMapUpdatedSignal(_player_info);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processExtraMapItemDestroyed(const MapItemDestroyedPacket& remove)
{
   const auto it = _map_items.find(remove.getUniqueId());

   if (it != _map_items.end())
   {
      MapItem& item = *it->second;
      item.setDestroyDirection(remove.getDirection());

      destroyMapItemSignal(item, remove.getIntensity());
      _map_items.erase(remove.getUniqueId());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processMapItemRemoved(const MapItemRemovedPacket& remove)
{
   const auto it = _map_items.find(remove.getUniqueId());

   if (it != _map_items.end())
   {
      removeMapItemSignal(*it->second);
      _map_items.erase(remove.getUniqueId());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::broadcastAddPlayerData()
{
   for (auto& [id, player_info] : _player_info)
   {
      // reset killed flag
      player_info.setKilled(false);

      // tell game drawable about players in the game
      addPlayerSignal(player_info.getId(), player_info.getNick(), player_info.getColor());
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::broadcastPlayerStartPositions()
{
   for (const auto& [id, player_info] : _player_info)
   {
      setPlayerPositionSignal(player_info.getId(), player_info.getX(), player_info.getY(), player_info.getAngle());
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::processStartGameResponse(const StartGameResponsePacket& response)
{
   qDebug("BombermanClient::data: game %d started", response.getId());

   setIngameMessagingActive(false);
   GameStateMachine::getInstance().setState(Constants::GameActive);

   _dead = false;

   gameStartedSignal();

   SoundManager& sm = SoundManager::getInstance();
   sm.playSoundStart();
   sm.restartPlayListAfterFadeOut(1000);

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
void BombermanClient::processStopGameResponse(const StopGameResponsePacket& response)
{
   qDebug("BombermanClient::data: game %d stopped, all rounds finished: %d", response.getId(), response.isFinished());

   GameStateMachine::getInstance().setState(Constants::GameStopped);

   _dead = true;

   if (response.isFinished())
   {
      gameStoppedSignal();

      SoundManager& sm = SoundManager::getInstance();
      sm.playSoundStart();
      sm.restartPlayListAfterFadeOut(SHOW_WINNER_TIME_SUM);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processGameEvent(const GameEventPacket& response)
{
   switch (response.getGameEvent())
   {
      case GameEventPacket::ExtraCollected:
      {
         SoundManager::getInstance().playSoundExtra();

         extraRemovedSignal(response.getX(), response.getY(), false, response.getExtraType(), response.getPlayerId());

         // rumble just a little on extra collect
         if (response.getPlayerId() == getPlayerId())
            rumbleSignal(0.2f, 500);

         break;
      }

      case GameEventPacket::ExtraDestroyed:
      {
         extraRemovedSignal(response.getX(), response.getY(), true, response.getExtraType(), -1);

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
void BombermanClient::processMessage(const MessagePacket& message_packet)
{
   messageReceivedSignal(message_packet.getSenderId(), message_packet.getMessage(), message_packet.isTypingFinished());

   if (message_packet.isTypingFinished())
   {
      // omit broadcast messages
      if (message_packet.getSenderId() != -1 && message_packet.getSenderId() != getPlayerId())
      {
         SoundManager::getInstance().playSoundMessageReceived();
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processTime(const TimePacket& time_packet)
{
   /*
      qDebug(
         "BombermanClient::data: server time: %s, left: %d",
         qPrintable(packet->getTimestamp().toString()),
         time_packet.getTimeLeft()
      );
   */

   if (const auto game_information = getCurrentGameInformation())
   {
      int time_left = time_packet.getTimeLeft();
      int duration = game_information->get().getDuration();

      if (time_left == (duration * 3 / 20))
      {
         SoundManager::getInstance().playSoundHurryUp();
      }

      timeChangedSignal(time_left, duration);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processCountdown(const CountdownPacket& countdown_packet)
{
   GameStateMachine::getInstance().setState(Constants::GamePreparing);

   int time_left = countdown_packet.getTimeLeft();

   qDebug("BombermanClient::processCountdown: left: %d", time_left);

   // sync tick
   if (time_left == SERVER_PREPARATION_TIME + SERVER_PREPARATION_SYNC_TIME - 1)
   {
      // use first tick to mute the music while the
      // countdown samples are played
      SoundManager::getInstance().fadeOut(1000);

      showGameSignal();

      broadcastAddPlayerData();
      broadcastPlayerStartPositions();
   }

   // next ticks
   if (time_left <= SERVER_PREPARATION_TIME - 1)
   {
      countdownSignal(time_left);

      // play sample
      SoundManager::getInstance().playSoundTime(time_left);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processPacket(const Packet& packet)
{
   switch (packet.getType())
   {
      case Packet::COUNTDOWN:
      {
         processCountdown(static_cast<const CountdownPacket&>(packet));
         break;
      }

      case Packet::CREATEGAMERESPONSE:
      {
         processCreateGameResponse(static_cast<const CreateGameResponsePacket&>(packet));
         break;
      }

      case Packet::DETONATION:
      {
         processDetonation(static_cast<const DetonationPacket&>(packet));
         break;
      }

      case Packet::ERROR:
      {
         processError(static_cast<const ErrorPacket&>(packet));
         break;
      }

      case Packet::EXTRAMAPITEMCREATED:
      {
         processExtraMapItemCreated(static_cast<const ExtraMapItemCreatedPacket&>(packet));
         break;
      }

      case Packet::GAMESTATS:
      {
         processGameStats(static_cast<const GameStatsPacket&>(packet));
         break;
      }

      case Packet::GAMEEVENT:
      {
         processGameEvent(static_cast<const GameEventPacket&>(packet));
         break;
      }

      case Packet::JOINGAMERESPONSE:
      {
         processJoinGameResponse(static_cast<const JoinGameResponsePacket&>(packet));
         break;
      }

      case Packet::KEY:
      {
         qWarning("BombermanClient::data(): wrong direction!");
         break;
      }

      case Packet::LISTGAMESRESPONSE:
      {
         processListGameResponse(static_cast<const ListGamesResponsePacket&>(packet));
         break;
      }

      case Packet::LOGINRESPONSE:
      {
         processLoginResponse(static_cast<const LoginResponsePacket&>(packet));
         break;
      }

      case Packet::PLAYERINFECTEDPACKET:
      {
         processPlayerInfected(static_cast<const PlayerInfectedPacket&>(packet));
         break;
      }

      case Packet::PLAYERKILLED:
      {
         processPlayerKilled(static_cast<const PlayerKilledPacket&>(packet));
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

         processPosition(static_cast<const PositionPacket&>(packet));
         break;
      }

      case Packet::LEAVEGAMERESPONSE:
      {
         processLeaveGameResponse(static_cast<const LeaveGameResponsePacket&>(packet));
         break;
      }

      case Packet::MAPITEMCREATED:
      {
         processMapItemCreated(static_cast<const MapItemCreatedPacket&>(packet));
         break;
      }

      case Packet::MAPITEMDESTROYED:
      {
         processExtraMapItemDestroyed(static_cast<const MapItemDestroyedPacket&>(packet));
         break;
      }

      case Packet::MAPITEMMOVE:
      {
         processMapItemMove(static_cast<const MapItemMovePacket&>(packet));
         break;
      }

      case Packet::MAPITEMREMOVED:
      {
         processMapItemRemoved(static_cast<const MapItemRemovedPacket&>(packet));
         break;
      }

      case Packet::MESSAGE:
      {
         processMessage(static_cast<const MessagePacket&>(packet));
         break;
      }

      case Packet::STARTGAMERESPONSE:
      {
         processStartGameResponse(static_cast<const StartGameResponsePacket&>(packet));
         break;
      }

      case Packet::STOPGAMERESPONSE:
      {
         processStopGameResponse(static_cast<const StopGameResponsePacket&>(packet));
         break;
      }

      case Packet::TIME:
      {
         processTime(static_cast<const TimePacket&>(packet));
         break;
      }

      case Packet::EXTRASHAKE:
      {
         processExtraShake(static_cast<const ExtraShakePacket&>(packet));
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

   // if playback is activated, record packets
   if (GamePlayback::getInstance().isRecording())
   {
      GamePlayback::getInstance().record(packet);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::readData()
{
   std::array<char, 4096> chunk{};
   int bytes_read;

   while ((bytes_read = NET_ReadFromStreamSocket(_socket.get(), chunk.data(), static_cast<int>(chunk.size()))) > 0)
   {
      _buffer.append(std::span(chunk).first(static_cast<size_t>(bytes_read)));
   }

   if (bytes_read < 0)
   {
      _socket.reset();
      clientDisconnect();
      return;
   }

   while (packetAvailable())
   {
      // block was read completely
      BinaryReader in = _buffer.reader();
      auto packet = Packet::deserialize(in);
      _buffer.consume(in.pos());

      if (packet)
      {
         processPacket(*packet);
      }

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
      NET_Status status = NET_GetAddressStatus(_address.get());

      if (status == NET_SUCCESS)
      {
         _socket.reset(NET_CreateClient(_address.get(), 6300, 0));
         _address.reset();

         if (!_socket)
         {
            reportConnectionError(SDL_GetError());
         }
      }
      else if (status == NET_FAILURE)
      {
         reportConnectionError(SDL_GetError());
         _address.reset();
      }

      return;
   }

   if (_socket && !isConnected())
   {
      NET_Status status = NET_GetConnectionStatus(_socket.get());

      if (status == NET_SUCCESS)
      {
         clientConnect();
         connectedSignal();
      }
      else if (status == NET_FAILURE)
      {
         reportConnectionError(SDL_GetError());
         _socket.reset();
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
   GameSettings::ControllerSettings& controller_settings = GameSettings::getInstance().getControllerSettings();

   bool control_key = event.key() == controller_settings.getUpKey() || event.key() == controller_settings.getDownKey() ||
                      event.key() == controller_settings.getLeftKey() || event.key() == controller_settings.getRightKey() ||
                      event.key() == controller_settings.getBombKey() || event.key() == controller_settings.getZoomInKey() ||
                      event.key() == controller_settings.getZoomOutKey() || event.key() == controller_settings.getStartKey();

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
   _current_player_id.reset();
   _player_info.clear();
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
*/
void BombermanClient::removePlayerInfo(int id)
{
   if (_player_info.erase(id) > 0 && _current_player_id == id)
   {
      _current_player_id.reset();
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processLeaveGameResponse(const LeaveGameResponsePacket& leave_packet)
{
   // either we left the game, so we have to clear the player info map
   // completely, or it was just one player, so the player map keeps intact
   if (leave_packet.getPlayerId() == getPlayerId())
   {
      clearPlayerInfoMap();
   }
   else
   {
      // remove player from playerinfo-map
      removePlayerInfo(leave_packet.getPlayerId());
   }

   // send infomap to all instances interested in them
   playerInfoMapUpdatedSignal(_player_info);

   // remove player
   removePlayerSignal(leave_packet.getPlayerId());

   // play "player left sample"
   if (!GamePlayback::getInstance().isReplaying())
      SoundManager::getInstance().playSoundPlayerLeft();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BombermanClient::processExtraShake(const ExtraShakePacket& shake_packet)
{
   if (const auto item = getMapItem(shake_packet.getMapItemUniqueId()))
   {
      shakeBlockSignal(*item);
      SoundManager::getInstance().playSoundBoxShake();
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
   send(kPacket);
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
      if (GameStateMachine::getInstance().getState() == Constants::GameActive)
      {
         toggleIngameMessaging();
      }
   }

   // write text
   if (isIngameMessagingActive())
   {
      if (GameStateMachine::getInstance().getState() == Constants::GameActive)
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
      if (GameStateMachine::getInstance().getState() == Constants::GamePreparing)
      {
         SoundManager& sm = SoundManager::getInstance();
         sm.restartPlayListAfterFadeOut(1000);
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

      GameSettings::ControllerSettings& controller_settings = GameSettings::getInstance().getControllerSettings();

      if (key == controller_settings.getUpKey())
      {
         _keys_pressed |= Constants::KeyUp;
         moved = true;
      }
      else if (key == controller_settings.getDownKey())
      {
         _keys_pressed |= Constants::KeyDown;
         moved = true;
      }
      else if (key == controller_settings.getLeftKey())
      {
         _keys_pressed |= Constants::KeyLeft;
         moved = true;
      }
      else if (key == controller_settings.getRightKey())
      {
         _keys_pressed |= Constants::KeyRight;
         moved = true;
      }
      else if (key == controller_settings.getBombKey()
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
   GameSettings::ControllerSettings& controller_settings = GameSettings::getInstance().getControllerSettings();

   if (key == controller_settings.getUpKey())
   {
      _keys_pressed &= ~Constants::KeyUp;
   }
   else if (key == controller_settings.getDownKey())
   {
      _keys_pressed &= ~Constants::KeyDown;
   }
   else if (key == controller_settings.getLeftKey())
   {
      _keys_pressed &= ~Constants::KeyLeft;
   }
   else if (key == controller_settings.getRightKey())
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
   else if (key == controller_settings.getBombKey())
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
      send(kPacket);
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
      removePlayerSignal(p.getId());
   }

   clearPlayerInfoMap();
   _games.clear();

   setGameId(-1);

   GameStateMachine::getInstance().setState(Constants::GameStopped);
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
   send(login);
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

   send(packet);

   if (finishedTyping)
   {
      setMessage("");
      SoundManager::getInstance().playSoundMessageSent();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::listGames()
{
   if (isConnected() && (GameStateMachine::getInstance().getState() != Constants::GameActive))
   {
      ListGamesRequestPacket packet;
      send(packet);
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
   bool dry_run = GameSettings::getInstance().getDevelopmentSettings().isDryRunEnabled();

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

   send(packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::joinGame(int game)
{
   JoinGameRequestPacket packet(game, _preferred_color);
   send(packet);
}

//-----------------------------------------------------------------------------
/*!
   \param color color to ask for when joining, none to take the next free one
*/
void BombermanClient::setPreferredColor(std::optional<Constants::Color> color)
{
   _preferred_color = color;
}

//-----------------------------------------------------------------------------
/*!
   \param game game to start
*/
void BombermanClient::startGame(int game)
{
   GameStateMachine::getInstance().setState(Constants::GamePreparing);

   StartGameRequestPacket packet(game);
   send(packet);
}

//-----------------------------------------------------------------------------
/*!
   \param game game to stop
*/
void BombermanClient::stopGame(int game)
{
   StopGameRequestPacket request(game);
   send(request);
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
   const bool connected_or_connecting = static_cast<bool>(_socket) || static_cast<bool>(_address);

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
   send(packet);

   // the other local players leave with us
   leaveGameSignal();
}

//-----------------------------------------------------------------------------
/*!
   \param ids player ids of the other players on this machine
*/
void BombermanClient::setLocalPlayerIds(const std::vector<int>& ids)
{
   _local_player_ids = ids;
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
   \return true for this client's own player and the other players on this machine
*/
bool BombermanClient::isLocalPlayer(int id) const
{
   return id == getPlayerId() || std::ranges::find(_local_player_ids, id) != _local_player_ids.end();
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
         HelpManager::getInstance().addMessage("", TEXT_ERROR_UNABLE_TO_BIND, Constants::HelpSeverityError);

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
         bots = GameSettings::getInstance().getCreateGameSettingsSingle().getBotCount();
      else
         bots = GameSettings::getInstance().getCreateGameSettingsMulti().getBotCount();

      _bot_factory->add(bots);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BombermanClient::initializePlayback()
{
   bool recording = GameSettings::getInstance().getDevelopmentSettings().isGameRecordingEnabled();

   GamePlayback::getInstance().setRecording(recording);
}

//-----------------------------------------------------------------------------
/*!
   \return local ips
*/
std::vector<std::string> BombermanClient::getLocalIps() const
{
   std::vector<std::string> ips;

   int count = 0;
   const std::unique_ptr<NET_Address*[], decltype(&NET_FreeLocalAddresses)> addresses(
      NET_GetLocalAddresses(&count), &NET_FreeLocalAddresses
   );

   if (addresses)
   {
      for (NET_Address* address : std::span(addresses.get(), static_cast<size_t>(count)))
      {
         const char* address_text = NET_GetAddressString(address);

         if (!address_text)
            continue;

         std::string ip(address_text);

         // IPv4 only, no loopback
         if (ip.find(':') == std::string::npos && ip != "127.0.0.1")
            ips.push_back(ip);
      }
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
   if (GameStateMachine::getInstance().getState() == Constants::GameStopped)
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
      if (GameStateMachine::getInstance().getState() == Constants::GameStopped)
      {
         // check if we're in main menu right now
         if (isMainMenuActive())
         {
            GamePlayback::getInstance().playDemo();
         }
      }
   }
   else
   {
      if (GamePlayback::getInstance().isReplaying())
      {
         GamePlayback::getInstance().abort();
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
   if (GameStateMachine::getInstance().getState() == Constants::GameStopped)
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
         HelpManager::getInstance().addMessage("", ip_text, Constants::HelpSeverityNotification);
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
