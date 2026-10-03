#include "botclient.h"

// shared
#include "botbombmapitem.h"
#include "extramapitem.h"
#include "logging.h"
#include "mapitem.h"

// SDL
#include <SDL3_net/SDL_net.h>

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

#include <array>
#include <span>

BotClient::BotClient() = default;

BotClient::~BotClient()
{
   // the bot accesses the player infos, so it goes first
   _bot.reset();
}

void BotClient::initialize()
{
   _poll_timer.timeoutSignal.connect([this]() { poll(); });
   _poll_timer.start(16);

   // init auto join/start
   initializeAutoJoinStart();
}

void BotClient::connectToServer()
{
   _address.reset(NET_ResolveHostname(_host.c_str()));

   if (!_address)
   {
      clientDisconnect();
   }
}

// poll for connection progress and incoming data, once per tick
void BotClient::poll()
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
            clientDisconnect();
         }
      }
      else if (status == NET_FAILURE)
      {
         _address.reset();
         clientDisconnect();
      }

      return;
   }

   if (_socket && !_connected)
   {
      NET_Status status = NET_GetConnectionStatus(_socket.get());

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
         _socket.reset();
         clientDisconnect();
      }

      return;
   }

   if (_socket && _connected)
   {
      readData();
   }
}

void BotClient::clientConnect()
{
   _connected = true;
}

void BotClient::clientDisconnect()
{
   _connected = false;
}

/*!
   \param host host to set
*/
void BotClient::setHost(const std::string& host)
{
   _host = host;
}

/*!
   \param nick player's nick
*/
void BotClient::setNick(const std::string& nick)
{
   _nick = nick;
}

void BotClient::initializeAutoJoinStart()
{
   if (_auto_join)
   {
      // login() itself fires from poll() once the connection succeeds - see connectToServer()
      updatePlayerIdSignal.connect([this](int) { selectGame(); });

      gameSelectedSignal.connect([this]() { joinGame(); });
   }
}

void BotClient::login()
{
   // reset player id
   _player_id = -1;

   // send login packet
   LoginRequestPacket login(_nick, true);
   send(login);
}

void BotClient::selectGame()
{
   ListGamesRequestPacket packet;
   send(packet);
}

/*!
   \param packet packet to send
*/
void BotClient::send(Packet& packet)
{
   packet.serialize();

   if (_socket)
   {
      NET_WriteToStreamSocket(_socket.get(), packet.data(), static_cast<int>(packet.size()));
   }
}

/*!
   \return true if sufficient data was received
*/
bool BotClient::packetAvailable()
{
   // blocksize not initialized yet
   if (_block_size == 0)
   {
      // not enough data to read blocksize?
      if (_buffer.bytesAvailable() < sizeof(uint16_t))
      {
         return false;
      }

      // read blocksize
      BinaryReader size_reader = _buffer.reader();
      size_reader >> _block_size;
      _buffer.consume(size_reader.pos());
   }

   // enough data?
   return (_buffer.bytesAvailable() >= _block_size);
}

/*!
   \param width map width
   \param height map height
*/
void BotClient::initBotMap(int width, int height)
{
   _bot->setBotMap(_bot->createMap(width, height));

   // connect botmap
   mapItemCreatedSignal.connect([this](const std::shared_ptr<MapItem>& item) { _bot->getBotMap().createMapItem(item); });

   mapItemRemovedSignal.connect([this](const MapItem& item) { _bot->getBotMap().removeMapItem(item); });
}

/*!
   \return bot
*/
Bot& BotClient::getBot() const
{
   return *_bot;
}

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

void BotClient::readData()
{
   std::array<char, 4096> chunk{};
   int bytes_read = 0;

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
         switch (packet->getType())
         {
            case Packet::CREATEGAMERESPONSE:
            {
               break;
            }

            case Packet::JOINGAMERESPONSE:
            {
               processJoinGameResponse(static_cast<const JoinGameResponsePacket&>(*packet));
               break;
            }

            case Packet::LEAVEGAMERESPONSE:
            {
               processLeaveGameResponse(static_cast<const LeaveGameResponsePacket&>(*packet));
               break;
            }

            case Packet::LISTGAMESRESPONSE:
            {
               processListGameResponse(static_cast<const ListGamesResponsePacket&>(*packet));
               break;
            }

            case Packet::LOGINRESPONSE:
            {
               processLoginResponse(static_cast<const LoginResponsePacket&>(*packet));
               break;
            }

            case Packet::PLAYERINFECTEDPACKET:
            {
               processPlayerInfected(static_cast<const PlayerInfectedPacket&>(*packet));
               break;
            }

            case Packet::PLAYERKILLED:
            {
               processPlayerKilled(static_cast<const PlayerKilledPacket&>(*packet));
               break;
            }

            case Packet::DETONATION:
            {
               break;
            }

            case Packet::ERROR:
            {
               break;
            }

            case Packet::POSITION:
            {
               processPosition(static_cast<const PositionPacket&>(*packet));
               break;
            }

            case Packet::MAPITEMCREATED:
            {
               processMapItemCreated(static_cast<const MapItemCreatedPacket&>(*packet));
               break;
            }

            case Packet::EXTRAMAPITEMCREATED:
            {
               processExtraMapItemCreated(static_cast<const ExtraMapItemCreatedPacket&>(*packet));
               break;
            }

            case Packet::MAPITEMDESTROYED:
            {
               processMapItemDestroyed(static_cast<const MapItemDestroyedPacket&>(*packet));
               break;
            }

            case Packet::MAPITEMREMOVED:
            {
               processMapItemRemoved(static_cast<const MapItemRemovedPacket&>(*packet));
               break;
            }

            case Packet::MAPITEMMOVE:
            {
               processMapItemMove(static_cast<const MapItemMovePacket&>(*packet));
               break;
            }

            case Packet::STARTGAMERESPONSE:
            {
               processStartGameResponse(static_cast<const StartGameResponsePacket&>(*packet));
               break;
            }

            case Packet::STOPGAMERESPONSE:
            {
               processStopGameResponse(static_cast<const StopGameResponsePacket&>(*packet));
               break;
            }

            case Packet::GAMEEVENT:
            {
               processGameEvent(static_cast<const GameEventPacket&>(*packet));
               break;
            }

            case Packet::MESSAGE:
            {
               break;
            }

            case Packet::TIME:
            {
               break;
            }

            case Packet::COUNTDOWN:
            {
               processCountdown(static_cast<const CountdownPacket&>(*packet));
               break;
            }

            case Packet::EXTRASHAKE:
            {
               processExtraShake(static_cast<const ExtraShakePacket&>(*packet));
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

/*!
   \param bot bot instance
*/
void BotClient::setBot(std::unique_ptr<Bot> bot)
{
   _bot = std::move(bot);
}

void BotClient::joinGame()
{
   if (_player_id != -1 && !isGameJoined() && !_games.empty())
   {
      JoinGameRequestPacket join_packet(getGameId());
      send(join_packet);

      PlayerSynchronizePacket sync_packet(PlayerSynchronizePacket::LevelLoaded);
      send(sync_packet);
   }
}

void BotClient::startGame()
{
   StartGameRequestPacket packet(_game_id);
   send(packet);
}

void BotClient::setGameId(int id)
{
   _game_id = id;
}

int BotClient::getGameId() const
{
   return _game_id;
}

void BotClient::setAutoJoin(bool enabled)
{
   _auto_join = enabled;
}

void BotClient::setAutoStart(bool enabled)
{
   _auto_start = enabled;
}

/*!
   \param packet packet to process
*/
void BotClient::processLoginResponse(const LoginResponsePacket& login_response)
{
   if (login_response.getId() >= 0)
   {
      _player_id = login_response.getId();

      // store server configuration data
      setServerConfiguration(login_response.getServerConfiguration());
      getBot().setServerConfiguration(login_response.getServerConfiguration());
   }

   updatePlayerIdSignal(_player_id);
}

/*!
   \param packet packet to process
*/
void BotClient::processJoinGameResponse(const JoinGameResponsePacket& response)
{
   int id = response.getPlayerId();

   if (!_player_info.contains(id))
   {
      auto new_info = std::make_unique<BotPlayerInfo>();
      new_info->setId(id);
      new_info->setNick(response.getNick());
      new_info->setColor(Constants::ColorWhite);  // TODO

      _player_info[id] = std::move(new_info);
   }

   // ensure it was us who joined as join game responses are
   // broadcasted to all players
   if (id == _player_id)
   {
      setGameJoined(true);

      _bot->setPlayerInfo(*_player_info.at(id));

      _game_id = response.getGameId();

      // select just the first one in the list
      GameInformation game_info = _games[0];
      createMap(game_info.getMapDimensions());
   }
}

/*!
   \param packet packet to process
*/
void BotClient::processLeaveGameResponse(const LeaveGameResponsePacket& response)
{
   if (response.getPlayerId() == _player_id)
   {
      removeSignal();
   }
}

/*!
   \param packet packet to process
*/
void BotClient::processMapItemCreated(const MapItemCreatedPacket& created_packet)
{
   std::shared_ptr<MapItem> item;

   switch (created_packet.getItemType())
   {
      case MapItem::Bomb:
      {
         int x = created_packet.getX();
         int y = created_packet.getY();

         auto bomb = std::make_shared<BotBombMapItem>(
            -1,  // we don't know the player id yet
            -1,  // we don't know the number of flames yet
            created_packet.getAppearance(),
            x,
            y
         );

         bomb->setUniqueId(created_packet.getUniqueId());

         if (const auto player = getPlayerInfo(created_packet.getPlayerId()))
         {
            bomb->setPlayerId(player->get().getId());
            bomb->setFlameCount(player->get().getFlameCount());
         }

         item = std::move(bomb);
         break;
      }

      default:
      {
         item = std::make_shared<MapItem>(created_packet);
         break;
      }
   }

   addMapItem(item);
   mapItemCreatedSignal(item);
}

/*!
   \param packet packet to process
*/
void BotClient::processExtraMapItemCreated(const ExtraMapItemCreatedPacket& created_packet)
{
   const std::shared_ptr<MapItem> extra = std::make_shared<ExtraMapItem>(created_packet);
   addMapItem(extra);
   mapItemCreatedSignal(extra);
}

/*!
   \param packet packet to process
*/
void BotClient::processMapItemDestroyed(const MapItemDestroyedPacket& destroyed_packet)
{
   queueObsoleteItem(destroyed_packet.getUniqueId());
}

/*!
   \param packet packet to process
*/
void BotClient::processMapItemRemoved(const MapItemRemovedPacket& removed_packet)
{
   queueObsoleteItem(removed_packet.getUniqueId());
}

/*!
   \param packet packet to process
*/
void BotClient::processMapItemMove(const MapItemMovePacket& move_packet)
{
   const auto item = getMapItem(move_packet.getMapItemId());

   if (item)
   {
      BotMap& bot_map = _bot->getBotMap();

      // in case the move direction is unknown, the item stopped its movement
      // and can be relocated at its nominal position
      if (move_packet.getDirection() == Constants::DirectionUnknown)
      {
         // relocate the bomb on the map
         int x = move_packet.getNominalX();
         int y = move_packet.getNominalY();

         // obsolete as done in the else section
         const auto& existing_item = bot_map.getItem(item->getX(), item->getY());
         if (existing_item && existing_item->getUniqueId() == item->getUniqueId())
         {
            bot_map.setItem(item->getX(), item->getY(), nullptr);
         }

         // remap item
         bot_map.setItem(x, y, item);

         item->setX(x);
         item->setY(y);
      }
      else
      {
         // notify bot about initiated kick, every bomb on a bot map is a BotBombMapItem
         if (item->getType() == MapItem::Bomb)
         {
            const auto& bomb = static_cast<const BotBombMapItem&>(*item);
            bombKickedSignal(item->getX(), item->getY(), move_packet.getDirection(), bomb.getFlames());
         }

         // this case will make the bot react as soon the bomb kick animation
         // has been started. by just taking care about the remapping case (see above)
         // the bot is only notified about the result of the bomb kick animation.
      }
   }
}

/*!
   \param id id of the map id
   \return map item, empty if unknown
*/
std::shared_ptr<MapItem> BotClient::getMapItem(int id) const
{
   auto item_iterator = _map_items.find(id);

   if (item_iterator != _map_items.end())
   {
      return item_iterator->second;
   }

   return {};
}

/*!
   \param map_item item to add
*/
void BotClient::addMapItem(std::shared_ptr<MapItem> map_item)
{
   auto& slot = _map_items[map_item->getUniqueId()];

   // a duplicate id must not free an item the bot map may still point to, retire it the regular way
   if (slot)
   {
      _obsolete_map_items.push(std::move(slot));
   }

   slot = std::move(map_item);
}

/*!
   \param id player id
   \return player info object, if known
*/
std::optional<std::reference_wrapper<BotPlayerInfo>> BotClient::getPlayerInfo(int id) const
{
   auto player_iterator = _player_info.find(id);

   if (player_iterator != _player_info.end())
   {
      return *player_iterator->second;
   }

   return std::nullopt;
}

/*!
   \return player info map
*/
const BotClient::PlayerInfoMap& BotClient::getPlayerInfoMap() const
{
   return _player_info;
}

/*!
   \param config reference to server configuration
*/
void BotClient::setServerConfiguration(const ServerConfiguration& config)
{
   _server_configuration = config;
}

/*!
   \return reference to server configuration
*/
const ServerConfiguration& BotClient::getServerConfiguration() const
{
   return _server_configuration;
}

/*!
   \param packet packet to process
*/
void BotClient::processPosition(const PositionPacket& position_packet)
{
   if (const auto info = getPlayerInfo(position_packet.getPlayerId()))
   {
      PlayerInfo& player_info = info->get();

      player_info.setPosition(position_packet.getX(), position_packet.getY(), position_packet.getAngle());

      player_info.setDirections(position_packet.getDirections());

      updatePlayerPositionSignal(position_packet.getPlayerId(), position_packet.getX(), position_packet.getY(), position_packet.getAngle());

      if (player_info.getId() == getPlayerId())
      {
         setSpeed(position_packet.getDeltaX() + position_packet.getDeltaY());

         setDeltaX(position_packet.getDeltaX());
         setDeltaY(position_packet.getDeltaY());
      }
   }
}

/*!
   \param packet packet to process
*/
void BotClient::processStartGameResponse(const StartGameResponsePacket& response)
{
   if (response.isStarted())
   {
      gameStartedSignal();
   }
}

/*!
   \param packet packet to process
*/
void BotClient::processGameEvent(const GameEventPacket& game_event_packet)
{
   int player_id = game_event_packet.getPlayerId();

   if (player_id != -1)
   {
      auto player_iterator = _player_info.find(player_id);

      if (player_iterator != _player_info.end())
      {
         BotPlayerInfo& player_info = *player_iterator->second;

         switch (game_event_packet.getExtraType())
         {
            case Constants::ExtraBomb:
               player_info.addBomb();
               break;

            case Constants::ExtraFlame:
               player_info.addFlame();
               break;

            case Constants::ExtraSpeedup:
               player_info.addSpeed();
               break;

            case Constants::ExtraKick:
               player_info.setKickEnabled(true);
               break;

            default:
               break;
         }
      }
   }
}

/*!
   \param packet packet to process
*/
void BotClient::processPlayerInfected(const PlayerInfectedPacket& infected_packet)
{
   int id = infected_packet.getPlayerId();

   auto player_iterator = _player_info.find(id);

   if (player_iterator != _player_info.end())
   {
      switch (infected_packet.getSkullType())
      {
         // infection aborted
         case Constants::SkullReset:
         {
            player_iterator->second->infect(nullptr);
            break;
         }

         // infection started
         default:
         {
            auto disease = std::make_unique<PlayerDisease>();
            disease->setType(infected_packet.getSkullType());

            // infect player
            player_iterator->second->infect(std::move(disease));
            break;
         }
      }
   }
}

/*!
   \param packet packet to process
*/
void BotClient::processPlayerKilled(const PlayerKilledPacket& killed_packet)
{
   int id = killed_packet.getPlayerId();

   auto player_iterator = _player_info.find(id);

   if (player_iterator != _player_info.end())
   {
      player_iterator->second->setKilled(true);
   }

   if (id == _player_id)
   {
      _bot->die();
   }
}

/*!
   \param packet stop game response packet
*/
void BotClient::processStopGameResponse(const StopGameResponsePacket& stop_game_packet)
{
   if (stop_game_packet.getId() == getGameId())
   {
      _bot->idle();
   }
}

/*!
   \param keys_pressed walk direction and bomb drop
*/
void BotClient::walk(int8_t keys_pressed)
{
   // keys_pressed and _keys_pressed had the same value (up) but the
   // bot was not moving at all.. no further key packets were sent
   // to the server since keys_pressed and _keys_pressed have been
   bool send_again = false;

   if ((keys_pressed & Constants::KeyUp) && getDeltaY() >= 0.0f)
   {
      send_again = true;
   }

   if ((keys_pressed & Constants::KeyDown) && getDeltaY() <= 0.0f)
   {
      send_again = true;
   }

   if ((keys_pressed & Constants::KeyLeft) && getDeltaX() >= 0.0f)
   {
      send_again = true;
   }

   if ((keys_pressed & Constants::KeyRight) && getDeltaX() <= 0.0f)
   {
      send_again = true;
   }

   if (keys_pressed != _keys_pressed || send_again)
   {
      _keys_pressed = keys_pressed;

      // cool bots don't get their keyboards flipped
      const BotPlayerInfo& info = getBot().getPlayerInfo();
      if (info.isInfected() && info.getDisease().getType() == Constants::SkullKeyboardInvert)
      {
         info.getDisease().applyKeyboardInvert(keys_pressed);
      }

      // build keypacket
      KeyPacket key_packet(_player_id, keys_pressed);
      send(key_packet);
   }
}

void BotClient::bomb()
{
   // only place bombs within some field center
   // because the server might be placing the bomb into
   // an undesired field otherwise
   _keys_pressed = 0;
   int x = _bot->getXField();
   int y = _bot->getYField();
   int8_t keys = Constants::KeyBomb;

   // mark the field hazardous until the bot re-communicated the
   // field status
   markHazardousTemporarySignal(x, y, 500, _bot->getPlayerInfo().getFlameCount());

   KeyPacket key_packet(_player_id, keys);
   send(key_packet);
}

void BotClient::processListGameResponse(const ListGamesResponsePacket& list)
{
   _games = list.getGames();

   gameSelectedSignal();
}

void BotClient::setGameJoined(bool joined)
{
   _game_joined = joined;
}

/*!
   \return \c true if player joined game
*/
bool BotClient::isGameJoined() const
{
   return _game_joined;
}

/*!
   \param speed player speed
*/
void BotClient::setSpeed(float speed)
{
   _speed = speed;
}

/*!
   \return player speed
*/
float BotClient::getSpeed() const
{
   return _speed;
}

/*!
   \return player id
*/
int BotClient::getPlayerId() const
{
   return _player_id;
}

/*!
   \param message message to send
   \param finished_typing typing finished flag
   \param receiver_id id of the message receiver
*/
void BotClient::sendMessage(const std::string& message, bool finished_typing, int receiver_id)
{
   if (isGameJoined())
   {
      MessagePacket packet(-1, message, finished_typing, receiver_id);

      send(packet);
   }
}

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

void BotClient::bugTrack1()
{
   if (_bomb_time.elapsed() < 500)
   {
      qDebug("bug");
   }
}

int BotClient::getWalkCount() const
{
   return _walk_count;
}

void BotClient::increaseWalkCount()
{
   _walk_count++;
}

void BotClient::resetWalkCount()
{
   _walk_count = 0;
}

/*!
   \return delta y
*/
float BotClient::getDeltaY() const
{
   return _delta_y;
}

/*!
   \param value delta y
*/
void BotClient::setDeltaY(float value)
{
   _delta_y = value;
}

/*!
   \return delta x
*/
float BotClient::getDeltaX() const
{
   return _delta_x;
}

/*!
   \param value delta x
*/
void BotClient::setDeltaX(float value)
{
   _delta_x = value;
}

/*!
   \param packet packet to process
*/
void BotClient::processCountdown(const CountdownPacket& countdown_packet)
{
   int time_left = countdown_packet.getTimeLeft();

   // sync tick
   if (time_left == SERVER_PREPARATION_TIME + SERVER_PREPARATION_SYNC_TIME - 1)
   {
      resetBot();
   }
}

/*!
   \param packet packet to process
*/
void BotClient::processExtraShake(const ExtraShakePacket& extra_shake_packet)
{
   extraShakeSignal(extra_shake_packet.getMapItemUniqueId());
}

/*!
   \param id id of the item that is queued to be removed later
*/
void BotClient::queueObsoleteItem(int id)
{
   // take it out of the lookup map right away - a duplicate destroy/remove notification for the
   // same id (e.g. a malformed or resent packet) would otherwise find it again and queue the same
   // already-obsolete item twice.
   auto item_iterator = _map_items.find(id);

   if (item_iterator != _map_items.end())
   {
      _obsolete_map_items.push(std::move(item_iterator->second));
      _map_items.erase(item_iterator);
   }
}

void BotClient::clearObsoleteItems()
{
   _obsolete_map_items = {};
}

void BotClient::deleteObsoleteMapItems()
{
   std::queue<std::shared_ptr<MapItem>> items = std::move(_obsolete_map_items);
   clearObsoleteItems();

   while (!items.empty())
   {
      const std::shared_ptr<MapItem> item = std::move(items.front());
      items.pop();

      // the bot map lets go of it as well
      mapItemRemovedSignal(*item);
   }
}
