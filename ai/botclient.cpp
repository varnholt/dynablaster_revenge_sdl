// header
#include "botclient.h"

// shared
#include "botbombmapitem.h"
#include "extramapitem.h"
#include "mapitem.h"

// Qt
#include "logging.h"

// SDL
#include <SDL3_net/SDL_net.h>

// math
#include <math.h>

// ai
#include "bot.h"
#include "botconstants.h"
#include "botmap.h"
#include "botplayerinfo.h"

// packets
#include "countdownpacket.h"
#include "extramapitemcreatedpacket.h"
#include "extrashakepacket.h"
#include "gameeventpacket.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "keypacket.h"
#include "leavegameresponsepacket.h"
#include "listgamesrequestpacket.h"
#include "listgamesresponsepacket.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "mapitemcreatedpacket.h"
#include "mapitemdestroyedpacket.h"
#include "mapitemmovepacket.h"
#include "mapitemremovedpacket.h"
#include "messagepacket.h"
#include "playerdisease.h"
#include "playerinfectedpacket.h"
#include "playerkilledpacket.h"
#include "playersynchronizepacket.h"
#include "positionpacket.h"
#include "startgamerequestpacket.h"
#include "startgameresponsepacket.h"
#include "stopgameresponsepacket.h"

// std
#include <stdio.h>
#include <stdlib.h>

//-----------------------------------------------------------------------------
/*!
   \param parent parent object
*/
BotClient::BotClient()
    : _socket(0),
      _address(nullptr),
      _connected(false),
      _block_size(0),
      _bot(0),
      _bot_map(0),
      _game_id(0),
      _auto_join(true),
      _auto_start(false),
      _player_id(0),
      _keys_pressed(0),
      _game_joined(false),
      _speed(0.0f),
      _delta_x(0.0f),
      _delta_y(0.0f)
{
}

//-----------------------------------------------------------------------------
/*!
 */
BotClient::~BotClient()
{
   delete _bot;

   // bot accesses player info ptrs, so delete them later
   for (const auto& [id, player_info] : _player_info)
      delete player_info;

   _player_info.clear();
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::initialize()
{
   _poll_timer.timeoutSignal.connect([this]() { poll(); });
   _poll_timer.start(16);

   // init auto join/start
   initializeAutoJoinStart();
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::connectToServer()
{
   _address = NET_ResolveHostname(_host.c_str());

   if (!_address)
   {
      clientDisconnect();
   }
}

//-----------------------------------------------------------------------------
/*!
   poll for connection progress and incoming data, once per tick
*/
void BotClient::poll()
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
            clientDisconnect();
         }
      }
      else if (status == NET_FAILURE)
      {
         NET_UnrefAddress(_address);
         _address = nullptr;
         clientDisconnect();
      }

      return;
   }

   if (_socket && !_connected)
   {
      NET_Status status = NET_GetConnectionStatus(_socket);

      if (status == NET_SUCCESS)
      {
         clientConnect();

         if (_auto_join)
         {
            login();
         }
      }
      else if (status == NET_FAILURE)
      {
         NET_DestroyStreamSocket(_socket);
         _socket = nullptr;
         clientDisconnect();
      }

      return;
   }

   if (_socket && _connected)
   {
      readData();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::clientConnect()
{
   _connected = true;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::clientDisconnect()
{
   _connected = false;
}

//-----------------------------------------------------------------------------
/*!
   \param host host to set
*/
void BotClient::setHost(const std::string& host)
{
   _host = host;
}

//-----------------------------------------------------------------------------
/*!
   \param player's nick
*/
void BotClient::setNick(const std::string& nick)
{
   _nick = nick;
}

//-----------------------------------------------------------------------------
/*!
   \param botmap bot map
*/
void BotClient::setBotMap(BotMap* botmap)
{
   _bot_map = botmap;
}

//----------------------------------------------------------------------------
/*!
 */
void BotClient::initializeAutoJoinStart()
{
   if (_auto_join)
   {
      // login() itself fires from poll() once the connection succeeds - see connectToServer()
      updatePlayerIdSignal.connect([this](int) { selectGame(); });

      gameSelectedSignal.connect([this]() { joinGame(); });
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::login()
{
   // reset player id
   _player_id = -1;

   // send login packet
   LoginRequestPacket login(_nick, true);
   send(&login);
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::selectGame()
{
   ListGamesRequestPacket packet;
   send(&packet);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to send
*/
void BotClient::send(Packet* packet)
{
   packet->serialize();

   if (_socket)
   {
      NET_WriteToStreamSocket(_socket, packet->constData(), static_cast<int>(packet->size()));
   }
}

//-----------------------------------------------------------------------------
/*!
   \param in datastream
   \return true if sufficient data was received
*/
bool BotClient::packetAvailable()
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

      // qDebug("BotClient::packetAvailable: %d bytes", _block_size);
   }

   // enough data?
   return (_buffer.bytesAvailable() >= _block_size);
}

//-----------------------------------------------------------------------------
/*!
   \param height map height
   \param width map width
*/
void BotClient::initBotMap(int width, int height)
{
   // delete _bot_map;
   _bot_map = _bot->createMap(width, height);
   _bot->setBotMap(_bot_map);

   // connect botmap
   mapItemCreatedSignal.connect([this](MapItem* item) { _bot_map->createMapItem(item); });

   mapItemRemovedSignal.connect([this](MapItem* item) { _bot_map->removeMapItem(item); });
}

//-----------------------------------------------------------------------------
/*!
   \return ptr to bot
*/
Bot* BotClient::getBot() const
{
   return _bot;
}

//-----------------------------------------------------------------------------
/*!
   \param dimensions dimensions to use
*/
void BotClient::createMap(Constants::Dimension dimensions)
{
   int width = 13;
   int height = 11;

   switch (dimensions)
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

   initBotMap(width, height);
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::readData()
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

      if (packet)
      {
         switch (packet->getType())
         {
            case Packet::CREATEGAMERESPONSE:
            {
               // processCreateGameResponse(packet.get());
               break;
            }

            case Packet::JOINGAMERESPONSE:
            {
               processJoinGameResponse(packet.get());
               break;
            }

            case Packet::LEAVEGAMERESPONSE:
            {
               processLeaveGameResponse(packet.get());
               break;
            }

            case Packet::LISTGAMESRESPONSE:
            {
               processListGameResponse(packet.get());
               break;
            }

            case Packet::LOGINRESPONSE:
            {
               processLoginResponse(packet.get());
               break;
            }

            case Packet::PLAYERINFECTEDPACKET:
            {
               processPlayerInfected(packet.get());
               break;
            }

            case Packet::PLAYERKILLED:
            {
               processPlayerKilled(packet.get());
               break;
            }

            case Packet::DETONATION:
            {
               // processDetonation(packet.get());
               break;
            }

            case Packet::ERROR:
            {
               break;
            }

            case Packet::POSITION:
            {
               processPosition(packet.get());
               break;
            }

            case Packet::MAPITEMCREATED:
            {
               processMapItemCreated(packet.get());
               break;
            }

            case Packet::EXTRAMAPITEMCREATED:
            {
               processExtraMapItemCreated(packet.get());
               break;
            }

            case Packet::MAPITEMDESTROYED:
            {
               processMapItemDestroyed(packet.get());
               break;
            }

            case Packet::MAPITEMREMOVED:
            {
               processMapItemRemoved(packet.get());
               break;
            }

            case Packet::MAPITEMMOVE:
            {
               processMapItemMove(packet.get());
               break;
            }

            case Packet::STARTGAMERESPONSE:
            {
               processStartGameResponse(packet.get());
               break;
            }

            case Packet::STOPGAMERESPONSE:
            {
               processStopGameResponse(packet.get());
               break;
            }

            case Packet::GAMEEVENT:
            {
               processGameEvent(packet.get());
               break;
            }

            case Packet::MESSAGE:
            {
               // processMessage(packet.get());
               break;
            }

            case Packet::TIME:
            {
               // processTime(packet.get());
               break;
            }

            case Packet::COUNTDOWN:
            {
               processCountdown(packet.get());
               break;
            }

            case Packet::EXTRASHAKE:
            {
               processExtraShake(packet.get());
               break;
            }

            case Packet::INVALID:
            {
               qWarning("BotClient::data: Packet::INVALID received");
               break;
            }

            default:
               break;
         }
      }

      _block_size = 0;
   }

   _buffer.compact();
}

//-----------------------------------------------------------------------------
/*!
   \param bot bot instance
*/
void BotClient::setBot(Bot* bot)
{
   _bot = bot;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::joinGame()
{
   if (_player_id != -1)
   {
      if (!isGameJoined())
      {
         if (!_games.empty())
         {
            // qDebug("BotClient::joinGame");
            JoinGameRequestPacket join_packet(getGameId());
            send(&join_packet);

            PlayerSynchronizePacket sync_packet(PlayerSynchronizePacket::LevelLoaded);
            send(&sync_packet);
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::startGame()
{
   StartGameRequestPacket packet(_game_id);
   send(&packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::setGameId(int id)
{
   _game_id = id;
}

//-----------------------------------------------------------------------------
/*!
 */
int BotClient::getGameId() const
{
   return _game_id;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::setAutoJoin(bool enabled)
{
   _auto_join = enabled;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::setAutoStart(bool enabled)
{
   _auto_start = enabled;
}

//-----------------------------------------------------------------------------
/*!
   \param p packet to process
*/
void BotClient::processLoginResponse(Packet* p)
{
   LoginResponsePacket* login_response = (LoginResponsePacket*)p;

   // qDebug("BotClient::processLoginResponse: id: %d", login_response->getId());

   if (login_response->getId() >= 0)
   {
      _player_id = login_response->getId();

      // store server configuration data
      setServerConfiguration(login_response->getServerConfiguration());
      getBot()->setServerConfiguration(login_response->getServerConfiguration());
   }

   updatePlayerIdSignal(_player_id);
}

//-----------------------------------------------------------------------------
/*!
   \param p packet to process
*/
void BotClient::processJoinGameResponse(Packet* p)
{
   // qDebug("BotClient::processJoinGameResponse");

   JoinGameResponsePacket* response = (JoinGameResponsePacket*)p;

   BotPlayerInfo* info = 0;
   int id = response->getPlayerId();

   if (!_player_info.contains(id))
   {
      info = new BotPlayerInfo();
      info->setId(id);
      info->setNick(response->getNick());
      info->setColor(Constants::ColorWhite);  // TODO

      _player_info[id] = info;

      // send infomap to all instances interested in them
      // playerInfoMapRequest();
   }

   // ensure it was us who joined as join game responses are
   // broadcasted to all players
   if (id == _player_id)
   {
      setGameJoined(true);

      if (info)
         _bot->setPlayerInfo(info);

      _game_id = response->getGameId();

      // select just the first one in the list
      GameInformation game_info = _games[0];
      createMap(game_info.getMapDimensions());
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processLeaveGameResponse(Packet* p)
{
   LeaveGameResponsePacket* response = dynamic_cast<LeaveGameResponsePacket*>(p);

   if (response)
   {
      if (response->getPlayerId() == _player_id)
      {
         removeSignal();
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processMapItemCreated(Packet* packet)
{
   MapItem* item = 0;
   MapItemCreatedPacket* created_packet = (MapItemCreatedPacket*)packet;

   switch (created_packet->getItemType())
   {
      case MapItem::Bomb:
      {
         int x = created_packet->getX();
         int y = created_packet->getY();
         BotPlayerInfo* player = 0;

         item = new BotBombMapItem(
            -1,  // we don't know the player id yet
            -1,  // we don't know the number of flames yet
            created_packet->getAppearance(),
            x,
            y
         );

         item->setUniqueId(created_packet->getUniqueId());

         player = getPlayerInfo(created_packet->getPlayerId());

         if (player)
         {
            ((BotBombMapItem*)item)->setPlayerId(player->getId());
            ((BotBombMapItem*)item)->setFlameCount(player->getFlameCount());

            /*
            if (DEBUG_BOMB_DROP)
            {
               qDebug(
                  "BotClient::processMapItemCreated: bomb dropped at (%d, %d), "
                  "flames: %d, estimated detonation time: %s",
                  item->getX(),
                  item->getY(),
                  player->getFlameCount(),
                  qPrintable(QTime::currentTime().addSecs(5).toString())
               );
            }
            */
         }

         break;
      }

      default:
      {
         item = new MapItem((MapItemCreatedPacket*)packet);
         break;
      }
   }

   addMapItem(item);
   mapItemCreatedSignal(item);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processExtraMapItemCreated(Packet* packet)
{
   ExtraMapItem* extra = new ExtraMapItem((ExtraMapItemCreatedPacket*)packet);
   addMapItem(extra);
   mapItemCreatedSignal(extra);
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processMapItemDestroyed(Packet* packet)
{
   MapItemDestroyedPacket* destroyed_packet = (MapItemDestroyedPacket*)packet;
   MapItem* item = getMapItem(destroyed_packet->getUniqueId());

   if (item)
   {
      //      qDebug(
      //         "BotClient::processMapItemDestroyed: player: %d, intensity: %f",
      //         destroyed_packet->getPlayerId(),
      //         destroyed_packet->getIntensity()
      //      );

      queueObsoleteItem(item);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processMapItemRemoved(Packet* packet)
{
   MapItemRemovedPacket* removed_packet = (MapItemRemovedPacket*)packet;
   MapItem* item = getMapItem(removed_packet->getUniqueId());

   if (item)
   {
      queueObsoleteItem(item);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processMapItemMove(Packet* packet)
{
   MapItemMovePacket* move_packet = (MapItemMovePacket*)packet;
   MapItem* item = getMapItem(move_packet->getMapItemId());

   if (item)
   {
      // in case the move direction is unknown, the item stopped its movement
      // and can be relocated at its nominal position
      if (move_packet->getDirection() == Constants::DirectionUnknown)
      {
         // relocate the bomb on the map
         int x = move_packet->getNominalX();
         int y = move_packet->getNominalY();

         // obsolete as done in the else section
         MapItem* existing_item = _bot_map->getItem(item->getX(), item->getY());
         if (existing_item && existing_item->getUniqueId() == item->getUniqueId())
         {
            _bot_map->setItem(item->getX(), item->getY(), 0);
         }

         // remap item
         _bot_map->setItem(x, y, item);

         item->setX(x);
         item->setY(y);
      }
      else
      {
         // notify bot about initiated kick
         BotBombMapItem* bomb = dynamic_cast<BotBombMapItem*>(item);

         if (bomb)
         {
            bombKickedSignal(item->getX(), item->getY(), move_packet->getDirection(), bomb->getFlames());
         }

         // this case will make the bot react as soon the bomb kick animation
         // has been started. by just taking care about the remapping case (see above)
         // the bot is only notified about the result of the bomb kick animation.

         // take bomb out of the map
         /*
         MapItem* existing_item = _bot_map->getItem(item->getX(), item->getY());

         if (
               existing_item
            && existing_item->getUniqueId() == item->getUniqueId()
         )
         {
            _bot_map->setItem(item->getX(), item->getY(), 0);
         }
         */
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param id id of the map id
   \param mapitem map item ptr
*/
MapItem* BotClient::getMapItem(int id) const
{
   MapItem* item = 0;

   auto it = _map_items.find(id);
   if (it != _map_items.end())
      item = it->second;

   return item;
}

//-----------------------------------------------------------------------------
/*!
   \param mapitem item to add
*/
void BotClient::addMapItem(MapItem* map_item)
{
   _map_items[map_item->getUniqueId()] = map_item;
}

//-----------------------------------------------------------------------------
/*!
   \param mapitem item to remove
*/
void BotClient::removeMapItem(MapItem* map_item)
{
   // delete item and take it from the mapitem-map - safe to delete synchronously here: this is
   // client-side bot-mirror bookkeeping reacting to a queued network packet, never nested
   // inside the item's own signal dispatch (these mirror objects don't run gameplay logic)
   _map_items.erase(map_item->getUniqueId());
   delete map_item;
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
   \return player info object
*/
BotPlayerInfo* BotClient::getPlayerInfo(int id) const
{
   auto it = _player_info.find(id);

   if (it != _player_info.end())
      return it->second;
   else
      return 0;
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
   \return player info object list
*/
std::vector<BotPlayerInfo*> BotClient::getPlayerInfoList() const
{
   std::vector<BotPlayerInfo*> list;
   list.reserve(_player_info.size());

   for (const auto& [id, player_info] : _player_info)
      list.push_back(player_info);

   return list;
}

//-----------------------------------------------------------------------------
/*!
   \return player info map
*/
std::map<int, BotPlayerInfo*>* BotClient::getPlayerInfoMap()
{
   return &_player_info;
}

//-----------------------------------------------------------------------------
/*!
   \param config reference to server configuration
*/
void BotClient::setServerConfiguration(const ServerConfiguration& config)
{
   _server_configuration = config;
}

//-----------------------------------------------------------------------------
/*!
   \return reference to server configuration
*/
const ServerConfiguration& BotClient::getServerConfiguration() const
{
   return _server_configuration;
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processPosition(Packet* packet)
{
   PositionPacket* position_packet = (PositionPacket*)packet;

   PlayerInfo* player_info = getPlayerInfo(position_packet->getPlayerId());

   if (player_info)
   {
      player_info->setPosition(position_packet->getX(), position_packet->getY(), position_packet->getAngle());

      player_info->setDirections(position_packet->getDirections());

      updatePlayerPositionSignal(position_packet->getPlayerId(), position_packet->getX(), position_packet->getY(), position_packet->getAngle());

      if (player_info->getId() == getPlayerId())
      {
         setSpeed(position_packet->getDeltaX() + position_packet->getDeltaY());

         setDeltaX(position_packet->getDeltaX());
         setDeltaY(position_packet->getDeltaY());
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processStartGameResponse(Packet* packet)
{
   StartGameResponsePacket* response = (StartGameResponsePacket*)packet;

   if (response->isStarted())
      gameStartedSignal();
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processGameEvent(Packet* packet)
{
   GameEventPacket* game_event_packet = (GameEventPacket*)packet;
   int player_id = game_event_packet->getPlayerId();

   if (player_id != -1)
   {
      //      qDebug(
      //         "processGameEvent: player: %d, picked extra: %d",
      //         player_id,
      //         game_event_packet->getExtraType()
      //      );

      if (_player_info.contains(player_id))
      {
         BotPlayerInfo* player_info = _player_info[player_id];

         switch (game_event_packet->getExtraType())
         {
            case Constants::ExtraBomb:
               player_info->addBomb();
               break;

            case Constants::ExtraFlame:
               player_info->addFlame();
               break;

            case Constants::ExtraSpeedup:
               player_info->addSpeed();
               break;

            case Constants::ExtraKick:
               player_info->setKickEnabled(true);
               break;

            default:
               break;
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processPlayerInfected(Packet* packet)
{
   PlayerInfectedPacket* infected_packet = (PlayerInfectedPacket*)packet;
   int id = infected_packet->getPlayerId();

   auto it = _player_info.find(id);

   if (it != _player_info.end())
   {
      switch (infected_packet->getSkullType())
      {
         // infection aborted
         case Constants::SkullReset:
         {
            it->second->infect(nullptr);
            break;
         }

         // infection started
         default:
         {
            auto disease = std::make_unique<PlayerDisease>();
            disease->setType(infected_packet->getSkullType());

            // infect player
            it->second->infect(std::move(disease));
            break;
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processPlayerKilled(Packet* packet)
{
   PlayerKilledPacket* killed_packet = (PlayerKilledPacket*)packet;
   int id = killed_packet->getPlayerId();

   auto it = _player_info.find(id);

   if (it != _player_info.end())
   {
      it->second->setKilled(true);
   }

   if (id == _player_id)
      _bot->die();
}

//-----------------------------------------------------------------------------
/*!
   \param packet stop game response packet
*/
void BotClient::processStopGameResponse(Packet* packet)
{
   StopGameResponsePacket* stop_game_packet = (StopGameResponsePacket*)packet;

   if (stop_game_packet->getId() == getGameId())
      _bot->idle();
}

//-----------------------------------------------------------------------------
/*!
   \param keys_pressed walk direction and bomb drop
*/
void BotClient::walk(int8_t keys_pressed)
{
   // keys_pressed and _keys_pressed had the same value (up) but the
   // bot was not moving at all.. no further key packets were sent
   // to the server since keys_pressed and _keys_pressed have been
   bool send_again = false;

   if (keys_pressed & Constants::KeyUp)
   {
      if (getDeltaY() >= 0.0f)
         send_again = true;
   }
   if (keys_pressed & Constants::KeyDown)
   {
      if (getDeltaY() <= 0.0f)
         send_again = true;
   }
   if (keys_pressed & Constants::KeyLeft)
   {
      if (getDeltaX() >= 0.0f)
         send_again = true;
   }
   if (keys_pressed & Constants::KeyRight)
   {
      if (getDeltaX() <= 0.0f)
         send_again = true;
   }

   if (keys_pressed != _keys_pressed || send_again)
   {
      _keys_pressed = keys_pressed;

      // cool bots don't get their keyboards flipped
      PlayerDisease* disease = getBot()->getPlayerInfo()->getDisease();
      if (disease)
      {
         if (disease->getType() == Constants::SkullKeyboardInvert)
         {
            disease->applyKeyboardInvert(keys_pressed);
         }
      }

      // build keypacket
      KeyPacket key_packet(_player_id, keys_pressed);
      send(&key_packet);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::bomb()
{
   // only place bombs within some field center
   // because the server might be placing the bomb into
   // an undesired field otherwise
   _keys_pressed = 0;
   int8_t keys = 0;
   int x = _bot->getXField();
   int y = _bot->getYField();
   //   float relX = _bot->getX() - x;
   //   float relY = _bot->getY() - y;
   //
   //   if (
   //         relX > 0.1f && relX < 0.9f
   //      && relY > 0.1f && relY < 0.9f
   //   )
   //   {
   keys = Constants::KeyBomb;

   // mark the field hazardous until the bot re-communicated the
   // field status
   markHazardousTemporarySignal(x, y, 500, _bot->getPlayerInfo()->getFlameCount());
   //   }
   //   else
   //   {
   //      // keys = _bot->getBotKeysPressed();
   //      keys = _bot->computeWalkKeys();
   //   }

   KeyPacket key_packet(_player_id, keys);
   send(&key_packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::processListGameResponse(Packet* packet)
{
   ListGamesResponsePacket* list = (ListGamesResponsePacket*)packet;
   _games = list->getGames();

   gameSelectedSignal();
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::setGameJoined(bool joined)
{
   _game_joined = joined;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if player joined game
*/
bool BotClient::isGameJoined() const
{
   return _game_joined;
}

//-----------------------------------------------------------------------------
/*!
   \param speed player speed
*/
void BotClient::setSpeed(float speed)
{
   _speed = speed;
}

//-----------------------------------------------------------------------------
/*!
   \return player speed
*/
float BotClient::getSpeed() const
{
   return _speed;
}

//-----------------------------------------------------------------------------
/*!
   \return player id
*/
int BotClient::getPlayerId() const
{
   return _player_id;
}

//-----------------------------------------------------------------------------
/*!
   \param message message to send
   \param receiver_id id of the message receiver
*/
void BotClient::sendMessage(const std::string& message, bool finished_typing, int receiver_id)
{
   if (isGameJoined())
   {
      MessagePacket packet(-1, message, finished_typing, receiver_id);

      send(&packet);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::resetBot()
{
   setDeltaX(0.0f);
   setDeltaY(0.0f);

   // reset keys pressed
   _keys_pressed = 0;

   // reset killed flag for all players
   for (const auto& [id, player_info] : _player_info)
   {
      player_info->setKilled(false);
      player_info->reset();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::bugTrack1()
{
   if (_bomb_time.elapsed() < 500)
   {
      qDebug("bug");
   }
}

//-----------------------------------------------------------------------------
/*!
 */
int BotClient::getWalkCount() const
{
   return _walk_count;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::increaseWalkCount()
{
   _walk_count++;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::resetWalkCount()
{
   _walk_count = 0;
}

//-----------------------------------------------------------------------------
/*!
   \return delta y
*/
float BotClient::getDeltaY() const
{
   return _delta_y;
}

//-----------------------------------------------------------------------------
/*!
   \param value delta y
*/
void BotClient::setDeltaY(float value)
{
   _delta_y = value;
}

//-----------------------------------------------------------------------------
/*!
   \return delta x
*/
float BotClient::getDeltaX() const
{
   return _delta_x;
}

//-----------------------------------------------------------------------------
/*!
   \param value delta x
*/
void BotClient::setDeltaX(float value)
{
   _delta_x = value;
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processCountdown(Packet* packet)
{
   CountdownPacket* countdown_packet = (CountdownPacket*)packet;

   int time_left = countdown_packet->getTimeLeft();

   /*
   qDebug(
      "BotClient::processCountdown: left: %d",
      time_left
   );
   */

   // sync tick
   if (time_left == SERVER_PREPARATION_TIME + SERVER_PREPARATION_SYNC_TIME - 1)
   {
      resetBot();
   }
}

//-----------------------------------------------------------------------------
/*!
   \param packet packet to process
*/
void BotClient::processExtraShake(Packet* packet)
{
   ExtraShakePacket* extra_shake_packet = (ExtraShakePacket*)packet;

   int item_id = extra_shake_packet->getMapItemUniqueId();

   /*
   qDebug(
      "BotClient::processExtraShake: item %d contains an extra",
      item_id
   );
   */

   extraShakeSignal(item_id);
}

//-----------------------------------------------------------------------------
/*!
   \param item item that is queued to be deleted later
*/
void BotClient::queueObsoleteItem(MapItem* item)
{
   // remove from the lookup map right away - a duplicate destroy/remove notification for the
   // same id (e.g. a malformed or resent packet) would otherwise find it again via getMapItem()
   // and queue the same already-obsolete pointer twice, use-after-freeing it on the second flush.
   _map_items.erase(item->getUniqueId());
   _obsolete_map_items.push(item);
}

//-----------------------------------------------------------------------------
/*!
 */
void BotClient::clearObsoleteItems()
{
   _obsolete_map_items = std::queue<MapItem*>();
}

//-----------------------------------------------------------------------------
/*!
   \param item item that is queued to be deleted later
*/
void BotClient::deleteObsoleteMapItems()
{
   std::queue<MapItem*> items = _obsolete_map_items;

   while (!items.empty())
   {
      MapItem* item = items.front();
      items.pop();

      mapItemRemovedSignal(item);
      removeMapItem(item);
   }

   clearObsoleteItems();
}
