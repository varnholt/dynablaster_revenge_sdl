// header
#include "game.h"

// server
#include "collisiondetection.h"
#include "extrashakepackethandler.h"
#include "extraspawn.h"

// shared
#include "bombkickanimation.h"
#include "bombmapitem.h"
#include "bombpacket.h"
#include "countdownpacket.h"
#include "detonationpacket.h"
#include "errorpacket.h"
#include "extramapitem.h"
#include "extramapitemcreatedpacket.h"
#include "gameeventpacket.h"
#include "gamestatspacket.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "keypacket.h"
#include "listgamesresponsepacket.h"
#include "logging.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "map.h"
#include "mapitemcreatedpacket.h"
#include "mapitemdestroyedpacket.h"
#include "mapitemmovepacket.h"
#include "mapitemremovedpacket.h"
#include "messagepacket.h"
#include "player.h"
#include "playerdisease.h"
#include "playerinfectedpacket.h"
#include "playerkilledpacket.h"
#include "playerstats.h"
#include "positionpacket.h"
#include "random.h"
#include "settings.h"
#include "startgamerequestpacket.h"
#include "startgameresponsepacket.h"
#include "stonemapitem.h"
#include "stopgamerequestpacket.h"
#include "stopgameresponsepacket.h"
#include "stringutils.h"
#include "timepacket.h"

// stdlib
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <functional>
#include <iterator>
#include <memory>
#include <numbers>
#include <ranges>
#include <unordered_set>
#include <vector>

// SDL
#include <SDL3_net/SDL_net.h>

int Game::_game_id_counter = 0;

Game::Game()
{
   qDebug("Game::Game: initializing");

   // init config
   Settings settings(SERVER_CONFIG_FILE_SERVER, Settings::IniFormat);

   _position_skip_count = settings.value("skip_positions", 1).toInt();
   _skip_countdown = settings.value("skip_countdown", false).toBool();
   BombMapItem::setTickTime(settings.value("tick_count", SERVER_BOMB_TICKTIME_DEFAULT).toInt());
   const bool shake_packets_enabled = settings.value("shake_packets", true).toBool();

   _max_speed = SERVER_DEFAULT_SPEED + (SERVER_SPEEDUP_INCREMENT * SERVER_MAX_SPEEDUPS);

   _sync_max_time = settings.value("player_sync_max_time", SERVER_PLAYER_SYNC_MAX_TIME).toInt();

   // shake packet handler
   _shake_packet_handler = std::make_unique<ExtraShakePacketHandler>();
   _shake_packet_handler->setGame(this);
   _shake_packet_handler->setEnabled(shake_packets_enabled);

   // collision detection
   _collision_detection = std::make_unique<CollisionDetection>();
   _collision_detection->setGame(this);

   _collision_detection->playerKicksBombSignal.connect([this](Player* player, MapItem* item, bool vertically_kicked, int keys_pressed)
                                                       { playerKicksBomb(player, item, vertically_kicked, keys_pressed); });

   _collision_detection->playerIdleSignal.connect([this](int8_t directions, Player* player) { playerIdle(directions, player); });

   _collision_detection->playerMoveSignal.connect([this](
                                                     Player* player, float assigned_x_position, float assigned_y_position, int8_t directions
                                                  ) { playerMove(player, assigned_x_position, assigned_y_position, directions); });

   // init skulls
   initSkullSetup();
}

Game::~Game()
{
   qDebug("Game::~Game");

   // a once-kicked bomb still on the map owns its kick animation - drop it here, otherwise
   // deleteAll() below frees it and the bomb frees it a second time when the map goes away
   if (_map)
   {
      for (int y = 0; y < _map->getHeight(); y++)
      {
         for (int x = 0; x < _map->getWidth(); x++)
         {
            if (auto* bomb = dynamic_cast<BombMapItem*>(_map->getItem(x, y)))
            {
               bomb->setBombKickAnimation(nullptr);
            }
         }
      }
   }

   // remove those animations first since they access _map
   BombKickAnimation::deleteAll();

   _map.reset();
}

void Game::singleShotWhileAlive(int32_t milliseconds, std::function<void()> callback)
{
   Timer::singleShot(
      milliseconds,
      [lifetime = std::weak_ptr<bool>(_lifetime), callback = std::move(callback)]()
      {
         if (!lifetime.expired())
         {
            callback();
         }
      }
   );
}

void Game::initSkullSetup()
{
   // reading the skulls from the configuration is disabled, it just creates unnecessary confusion

   // used skulls
   const bool skull_autofire = true;
   const bool skull_minimum_bomb = true;
   const bool skull_keyboard_invert = true;
   const bool skull_mushroom = true;
   const bool skull_invisible = true;
   const bool skull_invincible = true;

   // unused for now
   const bool skull_maximum_bomb = false;
   const bool skull_slow = false;
   const bool skull_fast = false;
   const bool skull_no_bomb = false;

   /*
      skull face ordering

      [0] red    -> autofire
      [1] blue   -> min bomb
      [2] purple -> keyboardinvert
      [3] dizzy  -> mushroom
      [4] green  -> invisible
      [5] gold   -> invincible
   */

   // used sides
   const int skull_autofire_side = 0;
   const int skull_minimum_bomb_side = 1;
   const int skull_keyboard_invert_side = 2;
   const int skull_mushroom_side = 3;
   const int skull_invisible_side = 4;
   const int skull_invincible_side = 5;

   // unused for now
   const int skull_maximum_bomb_side = -1;
   const int skull_slow_side = -1;
   const int skull_fast_side = -1;
   const int skull_no_bomb_side = -1;

   std::vector<Constants::SkullType> faces(6, Constants::SkullReset);

   const auto assign_face = [&faces](int side, Constants::SkullType type)
   {
      if (side > -1 && side < 6)
      {
         faces[static_cast<size_t>(side)] = type;
      }
   };

   assign_face(skull_autofire_side, Constants::SkullAutofire);
   assign_face(skull_keyboard_invert_side, Constants::SkullKeyboardInvert);
   assign_face(skull_mushroom_side, Constants::SkullMushroom);
   assign_face(skull_invisible_side, Constants::SkullInvisible);
   assign_face(skull_invincible_side, Constants::SkullInvincible);
   assign_face(skull_minimum_bomb_side, Constants::SkullMinimumBomb);
   assign_face(skull_maximum_bomb_side, Constants::SkullMaximumBomb);
   assign_face(skull_slow_side, Constants::SkullSlow);
   assign_face(skull_fast_side, Constants::SkullFast);
   assign_face(skull_no_bomb_side, Constants::SkullNoBomb);

   PlayerDisease::setSkullFaces(faces);

   // init set of supported skulls
   std::unordered_set<Constants::SkullType> supported_skulls;

   const auto support = [&supported_skulls](bool enabled, Constants::SkullType type)
   {
      if (enabled)
      {
         supported_skulls.insert(type);
      }
   };

   support(skull_autofire, Constants::SkullAutofire);
   support(skull_keyboard_invert, Constants::SkullKeyboardInvert);
   support(skull_mushroom, Constants::SkullMushroom);
   support(skull_invisible, Constants::SkullInvisible);
   support(skull_invincible, Constants::SkullInvincible);
   support(skull_minimum_bomb, Constants::SkullMinimumBomb);
   support(skull_maximum_bomb, Constants::SkullMaximumBomb);
   support(skull_slow, Constants::SkullSlow);
   support(skull_fast, Constants::SkullFast);
   support(skull_no_bomb, Constants::SkullNoBomb);

   PlayerDisease::setSupportedSkulls(supported_skulls);
}

void Game::initialize()
{
   qDebug("Game::initialize");

   initializeMap();
   initializeTimers();
}

void Game::initializeMap()
{
   // if there are still items in the destroyed map, clear that map.
   // that map is only processed in 'bombExploded()' which is only triggered
   // when the game is not stopped. therefore the may be items left in this
   // map after one round
   _destroyed_map_items.clear();

   // if there was a map before, delete it
   _map.reset();

   int width = 0;
   int height = 0;
   int stones = 0;

   float extra_count = 0.0f;
   int extra_bombs = static_cast<int32_t>(_create_game_data._extra_bomb_enabled);
   int extra_flames = static_cast<int32_t>(_create_game_data._extra_flame_enabled);
   int extra_kicks = static_cast<int32_t>(_create_game_data._extra_kick_enabled);
   int extra_speed_ups = static_cast<int32_t>(_create_game_data._extra_speedup_enabled);
   int extra_skulls = static_cast<int32_t>(_create_game_data._extra_skulls_enabled);

   const float extra_sum = (extra_bombs * SERVER_WEIGHT_BOMBS) + (extra_flames * SERVER_WEIGHT_FLAMES) +
                           (extra_kicks * SERVER_WEIGHT_KICKS) + (extra_speed_ups * SERVER_WEIGHT_SPEEDUPS) +
                           (extra_skulls * SERVER_WEIGHT_SKULLS);

   // 13x11 field: 16 extras
   // 26x22 field: 32 extras

   // create default map
   std::vector<Point> start_positions;

   if (_create_game_data._dimension == Constants::Dimension13x11)
   {
      extra_count = 16.0f;

      width = 13;
      height = 11;
      stones = 60;

      start_positions = {Point(0, 0), Point(12, 10), Point(12, 0), Point(0, 10), Point(6, 5)};
   }
   else if (_create_game_data._dimension == Constants::Dimension19x17)
   {
      extra_count = 32.0f;

      width = 19;
      height = 17;
      stones = 120;

      start_positions = {
         Point(0, 0),
         Point(width - 1, height - 1),
         Point(width - 1, 0),
         Point(0, height - 1),
         Point(0, 8),    // center left
         Point(18, 8),   // center right
         Point(6, 4),    // center quad top left
         Point(12, 4),   // center quad top right
         Point(6, 12),   // center quad bottom left
         Point(12, 12),  // center quad bottom right
      };
   }
   else if (_create_game_data._dimension == Constants::Dimension25x21)
   {
      extra_count = 32.0f;

      width = 25;
      height = 21;
      stones = 140;

      start_positions = {
         Point(0, 0),
         Point(width - 1, height - 1),
         Point(width - 1, 0),
         Point(0, height - 1),
         Point(12, 0),   // center top
         Point(12, 20),  // center bottom
         Point(7, 6),    // center quad top left
         Point(17, 6),   // center quad top right
         Point(7, 14),   // center quad bottom left
         Point(17, 14),  // center quad bottom right
      };
   }

   float value_bombs = 0.0f;
   float value_flames = 0.0f;
   float value_kicks = 0.0f;
   float value_speed_ups = 0.0f;
   float value_skulls = 0.0f;

   if (extra_bombs > 0)
   {
      value_bombs = extra_count * (SERVER_WEIGHT_BOMBS / extra_sum);
   }

   if (extra_flames > 0)
   {
      value_flames = extra_count * (SERVER_WEIGHT_FLAMES / extra_sum);
   }

   if (extra_kicks > 0)
   {
      value_kicks = extra_count * (SERVER_WEIGHT_KICKS / extra_sum);
   }

   if (extra_speed_ups > 0)
   {
      value_speed_ups = extra_count * (SERVER_WEIGHT_SPEEDUPS / extra_sum);
   }

   if (extra_skulls > 0)
   {
      value_skulls = extra_count * (SERVER_WEIGHT_SKULLS / extra_sum);
   }

   extra_bombs = static_cast<int32_t>(value_bombs);
   extra_flames = static_cast<int32_t>(value_flames);
   extra_kicks = static_cast<int32_t>(value_kicks);
   extra_speed_ups = static_cast<int32_t>(value_speed_ups);
   extra_skulls = static_cast<int32_t>(value_skulls);

   int loop = 0;
   while ((extra_bombs + extra_flames + extra_kicks + extra_speed_ups + extra_skulls) < extra_count && loop < 5)
   {
      if (loop == 0)
      {
         extra_bombs = static_cast<int32_t>(std::ceil(value_bombs));
      }
      if (loop == 1)
      {
         extra_flames = static_cast<int32_t>(std::ceil(value_flames));
      }
      if (loop == 2)
      {
         extra_kicks = static_cast<int32_t>(std::ceil(value_kicks));
      }
      if (loop == 3)
      {
         extra_speed_ups = static_cast<int32_t>(std::ceil(value_speed_ups));
      }
      if (loop == 4)
      {
         extra_skulls = static_cast<int32_t>(std::ceil(value_skulls));
      }

      loop++;
   }

   _map = Map::generateMap(
      width,            // width
      height,           // height
      stones,           // stones
      extra_bombs,      // bombs
      extra_flames,     // flames
      extra_speed_ups,  // speedups
      extra_kicks,      // kicks
      extra_skulls,     // skulls
      start_positions   // start positions
   );

   _immune_times.assign(static_cast<size_t>(width * height), 0);
}

void Game::initializeTimers()
{
   qDebug("Game::initializeTimers");

   // update timer
   _update_timer.timeoutSignal.connect([this]() { update(); });
   _update_timer.start(1000 / SERVER_HEARTBEAT_IN_HZ);

   // game time timer
   _game_time_update_timer.timeoutSignal.connect([this]() { processGameTime(); });
   _game_time_update_timer.start(1000);

   // game preparation timer
   _preparation_timer.setInterval(1000);
   _preparation_timer.timeoutSignal.connect([this]() { updatePrepareGame(); });
}

void Game::broadcastStartPositions()
{
   for (Player* current_player : _players | std::views::values)
   {
      _outgoing_packets.push_back(std::make_unique<PositionPacket>(
         current_player->getId(), Constants::KeyDown, current_player->getX(), current_player->getY(), std::numbers::pi_v<float> * 1.5f
      ));
   }
}

void Game::initializePlayerStartPositions()
{
   int start_position_index = 0;
   for (Player* current_player : _players | std::views::values)
   {
      // reset player's extras
      current_player->reset();
      current_player->getPlayerRotation()->reset();

      // reposition player
      const Point start_position = _map->getStartPosition(start_position_index);
      current_player->setX(start_position.x() + 0.5f);
      current_player->setY(start_position.y() + 0.5f);

      start_position_index++;
   }

   setStartPositionsInitialized(true);
}

void Game::broadcastCreateMapItems()
{
   for (auto& packet : _map->getMapItemCreatedPackets())
   {
      _outgoing_packets.push_back(std::move(packet));
   }
}

void Game::broadcastClearMapItems()
{
   for (auto& packet : _map->getMapItemRemovedPackets())
   {
      _outgoing_packets.push_back(std::move(packet));
   }
}

void Game::initMapRelatedItems()
{
   broadcastClearMapItems();

   // reinitialize the map
   initializeMap();

   // reinitialize players
   initializePlayerStartPositions();
   broadcastStartPositions();

   // broadcast new map
   broadcastCreateMapItems();
}

void Game::broadcastGameInformation()
{
   std::vector<GameInformation> games;
   games.push_back(getGameInformation());
   _outgoing_packets.push_back(std::make_unique<ListGamesResponsePacket>(games, true));
}

void Game::broadcastStartGame()
{
   _outgoing_packets.push_back(std::make_unique<StartGameResponsePacket>(_game_id, true));
}

void Game::startGame()
{
   qDebug("Game::startNewGame");

   // reset the number of players that left during the running game
   setGameOnlyPopulatedByBotsMessageShown(false);
   resetPlayersLeftTheGameCount();

   // reset the game time
   _game_time.restart();

   // set state machine to active
   setState(Constants::GameActive);

   _running = true;

   // increase the number of rounds played
   increaseGamesPlayed();

   // broadcast game time
   processGameTime();

   // broadcast new game information to all players
   broadcastGameInformation();

   // broadcast "started" to all players
   broadcastStartGame();
}

GameInformation Game::getGameInformation()
{
   return GameInformation(
      getId(),
      getPlayerCount(),
      getMaximumPlayerCount(),
      getName(),
      getLevelName(),
      getCreator()->getId(),
      getMapDimension(),
      getExtras(),
      getDuration(),
      getGamesPlayed(),
      getGameRound()->getCurrent(),
      getGameRound()->getCount(),
      isSpawnExtrasEnabled()
   );
}

//! the main update loop
void Game::update()
{
   updatePositions();

   // there is no point checking extra or bomb conditions while the game isn't
   // running. for position updates it is because after a game or prior its
   // start the player may have to rotate towards the camera should be stopped
   // by sending zero-speed packets
   if (getState() == Constants::GameActive)
   {
      // check if player collects extras
      updateExtras();

      // update field immunity
      updateImmuneTimes();

      // check if player drops bombs
      updateBombs();

      // check if infected players collide
      updateInfections();
   }

   // send outgoing packets
   sendBroadcastPackets();
}

void Game::updatePositions()
{
   updatePlayerPositions();
}

void Game::updatePlayerPositions()
{
   /*
      movement concept:

      if player wants to move x
         if x movement is allowed but y movement is not
            then
               move x
               correct y

      if player wants to move y
         if y movement is allowed but x movement is not
            then
               move y
               correct x

      => if x and y can be reached needs to be checked separately


             0.0, 0.0      1.0, 0.0      2.0, 0.0

   0.0, 0.0  +-------------+-------------+-------------+
             |  :::::::::  |             |             |
             |:::epsilon:::|             |             |
             |:::epsilon:::|             |             |
             |:::epsilon:::|             |             |
             |  :::::::::  |             |             |
   0.0, 1.0  +-------------+-------------+-------------+
             |             |/////////////|
             |             |/////////////|
             |             |/////////////|
             |             |/////////////|
             |             |/////////////|
   0.0, 2.0  +-------------+-------------+
             |             |
             |             |
             |             |
             |             |
             |             |
             +-------------+
   */

   if (isStartPositionInitialized())
   {
      for (Player* player : _player_sockets | std::views::values)
      {
         _collision_detection->process(player);
      }
   }
}

bool Game::isKickPossible(int x, int y, Constants::Direction kick_direction)
{
   // kick if possible if level dimensions not exceeded and
   // if there's no bomb, stone or block located in the kick
   // direction

   bool kick_possible = false;

   int check_offset_x = x;
   int check_offset_y = y;

   switch (kick_direction)
   {
      case Constants::DirectionUp:
         check_offset_y -= 1;
         break;
      case Constants::DirectionDown:
         check_offset_y += 1;
         break;
      case Constants::DirectionLeft:
         check_offset_x -= 1;
         break;
      case Constants::DirectionRight:
         check_offset_x += 1;
         break;
      default:
         break;
   }

   if (check_offset_x >= 0 && check_offset_x < getMap()->getWidth() && check_offset_y >= 0 && check_offset_y < getMap()->getHeight())
   {
      // check if there is an obstructing map item
      MapItem* item = getMap()->getItem(check_offset_x, check_offset_y);

      if (item)
      {
         if (item->getType() == MapItem::Extra)
         {
            kick_possible = true;
         }
      }
      else
      {
         kick_possible = true;
      }

      // check if there is an obstructing player
      if (kick_possible)
      {
         for (Player* other_player : _players | std::views::values)
         {
            if (other_player->isKilled())
            {
               continue;
            }

            const int player_x = static_cast<int32_t>(std::floor(other_player->getX()));
            const int player_y = static_cast<int32_t>(std::floor(other_player->getY()));

            // there is an obstructing player
            if (player_x == check_offset_x && player_y == check_offset_y)
            {
               /*
                  only block the kick bomb if the obstructing player
                  is really close enough to the bomb to be kicked.
                  the bomb is located at x,y, so the obstructing player
                  is at least 1.0 distance units away. if the position
                  of the potentially obstructing player is

                  kicking player
                   |
                 +---+---+---+
                 | P1|( )|  P2
                 +---+---+---+
                       |    |
                      0.5  obstructing player
                           0.8

                  the following code could be just removed if the
                  kicking still behaves too glitchy.
                  then just set kick_possible = false.
               */

               // horizontal kick movement
               if (kick_direction == Constants::DirectionLeft || kick_direction == Constants::DirectionRight)
               {
                  const float dx = std::fabs(other_player->getX() - x);

                  if (dx < 1.0f + SERVER_KICK_PLAYER_DISTANCE)
                  {
                     kick_possible = false;
                  }
               }

               // vertical kick movement
               if (kick_direction == Constants::DirectionUp || kick_direction == Constants::DirectionDown)
               {
                  const float dy = std::fabs(other_player->getY() - y);

                  if (dy < 1.0f + SERVER_KICK_PLAYER_DISTANCE)
                  {
                     kick_possible = false;
                  }
               }
            }
         }
      }
   }

   return kick_possible;
}

void Game::createKickAnimation(BombMapItem* kicked_bomb, Constants::Direction kick_direction)
{
   kicked_bomb->kickAnimationSignal.connect(
      [this, lifetime = std::weak_ptr<bool>(_lifetime), kicked_bomb](Constants::Direction direction, float speed)
      {
         if (!lifetime.expired())
         {
            bombKickedAnimation(kicked_bomb, direction, speed);
         }
      }
   );

   // a kick animation needs the playfield map for collision detection
   auto kick_animation_owner = std::make_unique<BombKickAnimation>();
   BombKickAnimation* kick_animation = kick_animation_owner.get();
   kick_animation->setDirection(kick_direction);
   kick_animation->setMap(getMap());

   // a kick animations needs to "know" about current player
   // positions so they bounce back once they hit another player
   for (Player* player : _players | std::views::values)
   {
      if (!player->isKilled())
      {
         kick_animation->updatePlayerPosition(player->getId(), player->getX(), player->getY());
      }
   }

   // the animation is short-lived, the signals below live for the whole match: Signal<> has
   // no auto-disconnect, so the animation disconnects itself on destruction
   auto position_changed_connection =
      _collision_detection->playerPositionChangedSignal.connect([kick_animation](int id, float x, float y)
                                                                { kick_animation->updatePlayerPosition(id, x, y); });
   auto player_killed_connection = playerKilledSignal.connect([kick_animation](int id) { kick_animation->removePlayerPosition(id); });
   auto player_leaves_connection = playerLeavesSignal.connect([kick_animation](int id) { kick_animation->removePlayerPosition(id); });

   kick_animation->addDestroyCallback(
      [this, lifetime = std::weak_ptr<bool>(_lifetime), position_changed_connection, player_killed_connection, player_leaves_connection]()
      {
         if (lifetime.expired())
         {
            return;
         }

         _collision_detection->playerPositionChangedSignal.disconnect(position_changed_connection);
         playerKilledSignal.disconnect(player_killed_connection);
         playerLeavesSignal.disconnect(player_leaves_connection);
      }
   );

   kicked_bomb->setBombKickAnimation(std::move(kick_animation_owner));
}

void Game::updateImmuneTimes()
{
   for (int32_t& immune_time : _immune_times)
   {
      immune_time = std::max(0, immune_time - (1000 / SERVER_HEARTBEAT_IN_HZ));
   }
}

void Game::makeFieldImmune(int x, int y)
{
   _immune_times[static_cast<size_t>(y * _map->getWidth() + x)] = SERVER_IMMUNE_FIELD_DELAY;
}

bool Game::isFieldImmune(int x, int y) const
{
   return _immune_times[static_cast<size_t>(y * _map->getWidth() + x)] > 0;
}

void Game::playerKicksBomb(Player* player, MapItem* item, bool vertically_kicked, int keys_pressed)
{
   if (!item || item->getType() != MapItem::Bomb || player->isKilled() || !player->isKickEnabled())
   {
      return;
   }

   auto* kicked_bomb = dynamic_cast<BombMapItem*>(item);

   if (!kicked_bomb)
   {
      return;
   }

   Constants::Direction kick_direction = Constants::DirectionUnknown;

   if (vertically_kicked)
   {
      kick_direction = (keys_pressed & Constants::KeyUp) ? Constants::DirectionUp : Constants::DirectionDown;
   }
   else
   {
      kick_direction = (keys_pressed & Constants::KeyLeft) ? Constants::DirectionLeft : Constants::DirectionRight;
   }

   if (isKickPossible(item->getX(), item->getY(), kick_direction))
   {
      if (!kicked_bomb->isKicked())
      {
         createKickAnimation(kicked_bomb, kick_direction);
      }
      else
      {
         // bomb has already been kicked once - getBombKickAnimation() can be null if the
         // animation already finished/exploded between kicks
         BombKickAnimation* kick_animation = kicked_bomb->getBombKickAnimation();

         if (kick_animation)
         {
            kick_animation->setDirection(kick_direction);
         }
      }

      kicked_bomb->kick();
   }
}

void Game::playerDiseaseStopped(PlayerDisease* disease)
{
   if (disease)
   {
      _outgoing_packets.push_back(std::make_unique<PlayerInfectedPacket>(disease->getPlayerId(), Constants::SkullReset));
   }
}

void Game::playerMove(Player* player, float assigned_x_position, float assigned_y_position, int8_t directions)
{
   float speed_x = 0.0;
   float speed_y = 0.0;

   if (!player->isKilled())
   {
      speed_x = assigned_x_position - player->getX();
      speed_y = assigned_y_position - player->getY();
   }

   _outgoing_packets.push_back(std::make_unique<PositionPacket>(
      player->getId(),
      directions,
      assigned_x_position,
      assigned_y_position,
      player->getPlayerRotation()->getAngle(),
      speed_x,
      speed_y,
      player->getPlayerRotation()->getAngleDelta(),
      player->getSpeed()
   ));

   player->setPositionSkipCounter(0);
   player->setKeysPressed(player->getKeysPressed());

   // reset idle packet flag
   _idle_packet_sent_set.erase(player);
}

void Game::playerIdle(int8_t directions, Player* player)
{
   if (!_idle_packet_sent_set.contains(player))
   {
      // send a zero-distance (speed=0.0 packet)
      _outgoing_packets.push_back(std::make_unique<PositionPacket>(
         player->getId(),
         directions,
         player->getX(),
         player->getY(),
         player->getPlayerRotation()->getAngle(),
         0.0f,
         0.0f,
         player->getPlayerRotation()->getAngleDelta(),
         0.0f
      ));

      _idle_packet_sent_set.insert(player);
   }
}

void Game::createInfection(
   Player* infected_player,
   Player* infecting_player,
   ExtraMapItem* extra,
   const std::vector<Constants::SkullType>& faces
)
{
   float extra_elapsed_time = 0.0f;

   if (extra)
   {
      extra_elapsed_time = extra->getElapsedTime();
   }

   auto disease_owner = std::make_unique<PlayerDisease>();
   PlayerDisease* disease = disease_owner.get();
   disease->setPlayerId(infected_player->getId());

   if (infecting_player)
   {
      // infecting player infects other player
      disease->setType(infecting_player->getDisease()->getType());
   }
   else
   {
      // find out which side the rotating cube is showing right now
      const int side_step = (static_cast<int32_t>(std::floor(extra_elapsed_time + 0.5f))) % 6;
      const Constants::SkullType skull_type = faces[static_cast<size_t>(side_step)];
      disease->setType(skull_type);
   }

   // if the infected player has been infected before, abort the old infection
   if (infected_player->getDisease())
   {
      infected_player->getDisease()->abort();
   }

   disease->activate();

   // abort the disease once the game changes to 'stopped' - the disease is owned by the player
   // and may outlive this game, so it disconnects itself on destruction
   const auto state_changed_connection = stateChangedSignal.connect([disease](Constants::GameState) { disease->abort(); });
   disease->addDestroyCallback(
      [this, lifetime = std::weak_ptr<bool>(_lifetime), state_changed_connection]()
      {
         if (!lifetime.expired())
         {
            stateChangedSignal.disconnect(state_changed_connection);
         }
      }
   );

   disease->stoppedSignal.connect(
      [this, lifetime = std::weak_ptr<bool>(_lifetime), disease]()
      {
         if (!lifetime.expired())
         {
            playerDiseaseStopped(disease);
         }
      }
   );

   auto packet = std::make_unique<PlayerInfectedPacket>(infected_player->getId(), disease->getType());

   infected_player->infect(std::move(disease_owner));

   // either add the id of the player that infected the other
   // or add the position of the extra the extra is contained in
   if (infecting_player)
   {
      packet->setInfectorId(infecting_player->getId());
   }
   else
   {
      packet->setExtraPos(extra->getX(), extra->getY());
   }

   _outgoing_packets.push_back(std::move(packet));
}

void Game::updateExtras()
{
   for (Player* player : _player_sockets | std::views::values)
   {
      // init player position
      const int x = static_cast<int32_t>(std::floor(player->getX()));
      const int y = static_cast<int32_t>(std::floor(player->getY()));

      MapItem* map_item = _map->getItem(x, y);

      if (!map_item || map_item->getType() != MapItem::Extra)
      {
         continue;
      }

      // update player stats
      player->increaseExtrasCollected();

      // evaluate extra and pass it to the player
      auto* extra = dynamic_cast<ExtraMapItem*>(map_item);

      switch (extra->getExtraType())
      {
         case Constants::ExtraBomb:
         {
            player->increaseBombCount();
            break;
         }

         case Constants::ExtraFlame:
         {
            player->increaseFlameCount();
            break;
         }

         case Constants::ExtraSpeedup:
         {
            player->setSpeed(std::min(player->getSpeed() + SERVER_SPEEDUP_INCREMENT, _max_speed));
            break;
         }

         case Constants::ExtraKick:
         {
            player->setKickEnabled(true);
            break;
         }

         case Constants::ExtraSkull:
         {
            const std::vector<Constants::SkullType> faces = extra->getSkullFaces();
            createInfection(player, nullptr, extra, faces);
            break;
         }
      }

      // play that lovely coin sample
      auto game_event_packet = std::make_unique<GameEventPacket>(GameEventPacket::ExtraCollected, 1.0f, x, y);

      game_event_packet->setPlayerId(player->getId());
      game_event_packet->setExtraType(extra->getExtraType());

      _outgoing_packets.push_back(std::move(game_event_packet));

      // remove extramapitem
      _outgoing_packets.push_back(std::make_unique<MapItemRemovedPacket>(extra));

      // remove extra from the map
      _map->setItem(x, y, nullptr);

      delete extra;

      // if the extra is removed here, remove it from the destroyed maps
      // also, so we don't have a dangling pointer in that list
      _destroyed_map_items.erase(extra);
   }
}

void Game::updateInfections()
{
   // it is only required to go through here if skulls are used
   if (!_create_game_data._extra_skulls_enabled)
   {
      return;
   }

   const std::vector<Player*> players = getPlayers();

   for (Player* player1 : players)
   {
      if (!player1->isInfected() || player1->isKilled())
      {
         continue;
      }

      for (Player* player2 : players)
      {
         // player must be someone else, "to be infected" must be not infected yet
         if (player2 == player1 || player2->isInfected() || player2->isKilled())
         {
            continue;
         }

         const Vec2 position1(player1->getX(), player1->getY());
         const Vec2 position2(player2->getX(), player2->getY());

         const Vec2 delta = position1 - position2;

         // if they are near enough to each other
         if (delta.length() < 0.3f)
         {
            // player2: the player who is now infected
            // player1: the one who was already infected
            createInfection(player2, player1, nullptr);
         }
      }
   }
}

void Game::updateBombs()
{
   for (Player* player : _player_sockets | std::views::values)
   {
      // check if player wants to drop a bomb
      if (!player->isBombKeyLocked())
      {
         continue;
      }

      const int x = static_cast<int32_t>(std::floor(player->getX()));
      const int y = static_cast<int32_t>(std::floor(player->getY()));
      MapItem* item = _map->getItem(x, y);

      // if player is allowed to drop more bombs
      if (!item && player->getBombsDroppedCount() < player->getBombCount())
      {
         player->setBombsDroppedCount(player->getBombsDroppedCount() + 1);

         // owned by the map
         auto* bomb = new BombMapItem(player->getId(), player->getFlameCount(), -1, x, y);

         bomb->explodedSignal.connect(
            [this, lifetime = std::weak_ptr<bool>(_lifetime)](BombMapItem* exploded_bomb, bool recursive)
            {
               if (!lifetime.expired())
               {
                  bombExploded(exploded_bomb, recursive);
               }
            }
         );

         _map->setItem(x, y, bomb);

         _outgoing_packets.push_back(std::make_unique<MapItemCreatedPacket>(bomb, player->getId()));
      }

      // remove bomb key once a bomb has been dropped
      player->setKeysPressed(player->getKeysPressed() & ~Constants::KeyBomb);

      player->setBombKeyLocked(false);
   }
}

void Game::updateStatsPlayerKilled(Player* killer, Player* victim)
{
   if (killer && killer != victim)
   {
      killer->increaseKills();
   }

   if (victim)
   {
      victim->increaseDeaths();
      victim->increaseSurvivalTime(getDuration() - getTimeLeft());
   }

   broadcastGameStats();
}

void Game::processPlayerWon(Player* player)
{
   if (player)
   {
      player->increaseWins();
      player->increaseSurvivalTime(getDuration() - getTimeLeft());
      player->setKeysPressed(Constants::KeyDown);
   }

   broadcastGameStats();
}

void Game::sendMessageToOwner(const std::string& message)
{
   sendPacket(getSocket(_creator), std::make_unique<MessagePacket>(_creator->getId(), message, true));
}

bool Game::isGameOnlyPopulatedByBotsMessageShown() const
{
   return _game_only_populated_by_bots_message_shown;
}

void Game::setGameOnlyPopulatedByBotsMessageShown(bool value)
{
   _game_only_populated_by_bots_message_shown = value;
}

void Game::processOnlyBotsLeft()
{
   qDebug("Game::processOnlyBotsLeft: the game is now only populated by bots");

   const std::string message = "The game is now only populated by bots. Press F10 to abort.";

   if (!isGameOnlyPopulatedByBotsMessageShown())
   {
      setGameOnlyPopulatedByBotsMessageShown(true);
      sendMessageToOwner(message);
   }

   /*
      what to do now?
      - player could send a StopGameRequestPacket
      - player could send a KillBotRequestPacket
   */
}

void Game::broadcastGameStats()
{
   std::vector<int> ids;
   std::vector<PlayerStats> overall_stats;
   std::vector<PlayerStats> round_stats;

   for (Player* player : _players | std::views::values)
   {
      ids.push_back(player->getId());
      overall_stats.push_back(*player->getOverallStats());
      round_stats.push_back(*player->getRoundStats());
   }

   _outgoing_packets.push_back(std::make_unique<GameStatsPacket>(ids, overall_stats, round_stats));
}

void Game::rotateDeadPlayerTowardsBomb(Constants::Direction detonation_direction, Player* player, int x, int y)
{
   // the bomb direction is our first choice
   std::vector<Constants::Direction> candidate_directions{detonation_direction};

   // the directions 90° to that one our 2nd
   switch (detonation_direction)
   {
      case Constants::DirectionLeft:
      case Constants::DirectionRight:
         candidate_directions.push_back(Constants::DirectionUp);
         candidate_directions.push_back(Constants::DirectionDown);
         break;
      case Constants::DirectionUp:
      case Constants::DirectionDown:
         candidate_directions.push_back(Constants::DirectionLeft);
         candidate_directions.push_back(Constants::DirectionRight);
         break;
      default:
         break;
   }

   Constants::Direction test_direction = Constants::DirectionUnknown;
   for (const Constants::Direction direction : candidate_directions)
   {
      int check_x = x;
      int check_y = y;
      bool hit_something = false;

      // check if "fall direction" is blocked
      switch (direction)
      {
         case Constants::DirectionLeft:
            check_x--;
            break;
         case Constants::DirectionRight:
            check_x++;
            break;
         case Constants::DirectionUp:
            check_y--;
            break;
         case Constants::DirectionDown:
            check_y++;
            break;
         default:
            break;
      }

      // check if the dead player will hit something
      if (check_x >= 0 && check_x < _map->getWidth() && check_y >= 0 && check_y < _map->getHeight())
      {
         MapItem* check_item = _map->getItem(check_x, check_y);

         if (check_item && check_item->isBlocking())
         {
            hit_something = true;
         }
      }

      if (!hit_something)
      {
         test_direction = direction;
         break;
      }
   }

   // it 3 other directions fail => let the player fall towards the bomb
   if (test_direction == Constants::DirectionUnknown)
   {
      switch (detonation_direction)
      {
         case Constants::DirectionLeft:
            test_direction = Constants::DirectionRight;
            break;
         case Constants::DirectionRight:
            test_direction = Constants::DirectionLeft;
            break;
         case Constants::DirectionUp:
            test_direction = Constants::DirectionDown;
            break;
         case Constants::DirectionDown:
            test_direction = Constants::DirectionUp;
            break;
         default:
            break;
      }
   }

   if (test_direction == Constants::DirectionRight)
   {
      player->setKeysPressed(Constants::KeyLeft);
   }
   else if (test_direction == Constants::DirectionLeft)
   {
      player->setKeysPressed(Constants::KeyRight);
   }
   else if (test_direction == Constants::DirectionUp)
   {
      player->setKeysPressed(Constants::KeyDown);
   }
   else if (test_direction == Constants::DirectionDown)
   {
      player->setKeysPressed(Constants::KeyUp);
   }
}

void Game::bombExploded(BombMapItem* bomb, bool /*unused*/)
{
   if (getState() == Constants::GameStopped)
   {
      return;
   }

   std::vector<std::unique_ptr<Packet>> remove_items;

   // player may drop one more bomb
   Player* player = nullptr;
   {
      const auto player_iterator = _players.find(bomb->getPlayerId());
      if (player_iterator != _players.end())
      {
         player = player_iterator->second;
      }
   }

   // player may already be killed :)
   if (player)
   {
      player->setBombsDroppedCount(player->getBombsDroppedCount() - 1);
   }

   const int flame_count = bomb->getFlames();
   const int x = bomb->getX();
   const int y = bomb->getY();

   const std::vector<Constants::Direction>* directions = &_direction_check_center;

   bool done_up = (bomb->getDetonationOrigin() == BombMapItem::Top);
   bool done_down = (bomb->getDetonationOrigin() == BombMapItem::Bottom);
   bool done_left = (bomb->getDetonationOrigin() == BombMapItem::Left);
   bool done_right = (bomb->getDetonationOrigin() == BombMapItem::Right);

   bool check_game_over = false;

   // init detonation packet information
   int detonation_up = flame_count;
   int detonation_down = flame_count;
   int detonation_left = flame_count;
   int detonation_right = flame_count;

   // simple stuff: if bomb covered an extra, destroy it
   MapItem* shadowed_item = bomb->getShadowedItem();
   if (shadowed_item)
   {
      remove_items.push_back(std::make_unique<MapItemDestroyedPacket>(
         shadowed_item, bomb->getPlayerId(), Constants::DirectionUnknown, static_cast<float>(flame_count)
      ));

      shadowed_item->setCurrentlyDestroyed(true);
      _destroyed_map_items.insert(shadowed_item);

      // create appropriate game event
      _outgoing_packets.push_back(std::make_unique<GameEventPacket>(GameEventPacket::ExtraDestroyed, 1.0f, x, y));
   }

   // go beginning from the center into each direction
   for (int flame_index = 0; flame_index <= flame_count; flame_index++)
   {
      // center only needs one direction to be checked
      // append the others, after the center element has been processed
      if (flame_index == 1)
      {
         directions = &_direction_check_all;
      }

      for (const Constants::Direction direction : *directions)
      {
         bool process_direction = true;

         int x_distance = x;
         int y_distance = y;

         // calculate next position
         switch (direction)
         {
            case Constants::DirectionUp:
            {
               if (done_up)
               {
                  process_direction = false;
               }

               y_distance = y - flame_index;
               break;
            }

            case Constants::DirectionDown:
            {
               if (done_down)
               {
                  process_direction = false;
               }

               y_distance = y + flame_index;
               break;
            }

            case Constants::DirectionLeft:
            {
               if (done_left)
               {
                  process_direction = false;
               }

               x_distance = x - flame_index;
               break;
            }

            case Constants::DirectionRight:
            {
               if (done_right)
               {
                  process_direction = false;
               }

               x_distance = x + flame_index;
               break;
            }

            default:
            {
               break;
            }
         }

         if (!process_direction || x_distance < 0 || y_distance < 0 || x_distance >= _map->getWidth() || y_distance >= _map->getHeight())
         {
            continue;
         }

         // check for players that are eventually killed
         for (Player* current_player : _players | std::views::values)
         {
            /*
               i had the issue that a position of 5.0 has been communicated
               to the bot which as been interpreted by floor as 4.9999999999
               i.e. as 4.
               so while the bot assumed to be at the right position (5.0)
               the game set him as killed. that's the reason to add just
               a little tolerance here.

                +------------------+------------------+------------------+
                                   |                  |
                                  4.0                5.0
                                   <------------------>

                                 | | |              | | |
                               4-e 4 4+e          4-e e 4+e


                   = >leads to next issue:

                      +-------------+
                      |             |
                      |      |      |
                      |      |      |
                      |     \_/     |
                      +-------------+
                      |             |
                      |             |
                      |             |
                      |             |
                      +-------------+ <- player is here => player is never hit
                      |             |
                      |             |
                      |             |
                      |             |
                      +-------------+
            */

            const int current_player_x = static_cast<int32_t>(std::floor(current_player->getX()));
            const int current_player_y = static_cast<int32_t>(std::floor(current_player->getY()));

            // kill player - once ;)
            if (current_player_x == x_distance && current_player_y == y_distance && !current_player->isKilled() &&
                !current_player->isInvincible())
            {
               current_player->setKilled(true);

               // rotate killed player towards the bomb that killed him
               rotateDeadPlayerTowardsBomb(direction, current_player, x_distance, y_distance);

               // if the bomb has been originally detonated by another player, then
               // we have a different killer. the player who dropped the first bomb
               // in the detonation chain is considered "the killer".
               Player* killer = player;

               if (bomb->getIgniterId() != -1 && bomb->getIgniterId() != player->getId())
               {
                  killer = nullptr;
                  const auto igniter_iterator = _players.find(bomb->getIgniterId());
                  if (igniter_iterator != _players.end())
                  {
                     killer = igniter_iterator->second;
                  }
               }

               // update stats
               updateStatsPlayerKilled(killer, current_player);

               playerKilledSignal(current_player->getId());

               // create player killed packet
               _outgoing_packets.push_back(
                  std::make_unique<PlayerKilledPacket>(current_player->getId(), bomb->getPlayerId(), direction, bomb->getFlames())
               );

               // show player killed message
               broadcastMessage(std::format("{} was killed.", current_player->getNick()));

               // check if game is over
               check_game_over = true;
            }
         }

         // ignite bombs that are kicked right into a detonation
         BombKickAnimation::ignite(x_distance, y_distance);

         // check for mapitems that need to be removed
         MapItem* map_item = _map->getItem(x_distance, y_distance);

         // if a mapitem is found or the field is immune, stop the
         // detonation
         bool stop_detonation = false;
         if (isFieldImmune(x_distance, y_distance))
         {
            stop_detonation = true;
         }

         // if there is a mapitem in the way
         // which is not the bomb that just exploded, then continue
         else if (map_item && map_item != bomb)
         {
            stop_detonation = true;

            if (map_item->isDestroyable() && !map_item->isCurrentlyDestroyed() && map_item->getType() != MapItem::Bomb)
            {
               // map item was destroyed
               remove_items.push_back(
                  std::make_unique<MapItemDestroyedPacket>(map_item, bomb->getPlayerId(), direction, static_cast<float>(flame_count))
               );

               if (map_item->getType() == MapItem::Extra)
               {
                  // create appropriate game event
                  _outgoing_packets.push_back(
                     std::make_unique<GameEventPacket>(GameEventPacket::ExtraDestroyed, 1.0f, map_item->getX(), map_item->getY())
                  );
               }

               // mark the current item as "is being destroyed"
               //
               // background:
               //
               //    if surrounding bombs get initiated by the
               //    currently detonating bomb, they musn't destroy
               //    any more surrounding mapitems. therefore the
               //    mapitem that was destroyed shall last a little
               //    while in order to block the other bombs.
               map_item->setCurrentlyDestroyed(true);

               _destroyed_map_items.insert(map_item);
            }

            // initiate surrounding bombs
            if (map_item->getType() == MapItem::Bomb && !map_item->isCurrentlyDestroyed())
            {
               // neighboured bombs explode from another bomb's explosion
               auto* neighbour_bomb = dynamic_cast<BombMapItem*>(map_item);

               // origin: left
               if (neighbour_bomb->getX() > map_item->getX())
               {
                  neighbour_bomb->setDetonationOrigin(BombMapItem::Left);
               }

               // origin: right
               if (neighbour_bomb->getX() < map_item->getX())
               {
                  neighbour_bomb->setDetonationOrigin(BombMapItem::Right);
               }

               // origin: top
               if (neighbour_bomb->getY() > map_item->getY())
               {
                  neighbour_bomb->setDetonationOrigin(BombMapItem::Top);
               }

               // origin: bottom
               if (neighbour_bomb->getY() < map_item->getY())
               {
                  neighbour_bomb->setDetonationOrigin(BombMapItem::Bottom);
               }

               // trigger recursive explosion
               if (neighbour_bomb->getInterval() > SERVER_BOMB_NEIGHBOUR_DELAY)
               {
                  // the id of the player who ignited another bomb
                  // with his own one is inherited here; this is done
                  // in order to compute a proper score for the kills.
                  neighbour_bomb->setIgniterId(bomb->getPlayerId());

                  // reduce timer interval in order to make the bomb
                  // explode pretty soon
                  neighbour_bomb->setInterval(SERVER_BOMB_NEIGHBOUR_DELAY);
               }
            }
         }

         // in any case: stop explosion into the given direction
         if (stop_detonation)
         {
            switch (direction)
            {
               case Constants::DirectionUp:
               {
                  done_up = true;
                  detonation_up = flame_index;
                  break;
               }

               case Constants::DirectionDown:
               {
                  done_down = true;
                  detonation_down = flame_index;
                  break;
               }

               case Constants::DirectionLeft:
               {
                  done_left = true;
                  detonation_left = flame_index;
                  break;
               }

               case Constants::DirectionRight:
               {
                  done_right = true;
                  detonation_right = flame_index;
                  break;
               }

               default:
               {
                  break;
               }
            }
         }
      }
   }

   // cut off explosions on the map's borders
   // this is just done for visual purposes
   if (x + detonation_right >= _map->getWidth())
   {
      detonation_right -= (x + detonation_right - _map->getWidth() + 1);
   }

   if (x - detonation_left < 0)
   {
      detonation_left = x;
   }

   if (y + detonation_down >= _map->getHeight())
   {
      detonation_down -= (y + detonation_down - _map->getHeight() + 1);
   }

   if (y - detonation_up < 0)
   {
      detonation_up = y;
   }

   // a destroyed item may currently be some other, still-kicked bomb's shadowed item (the item
   // it's temporarily covering on the grid) - that raw reference has to be invalidated before
   // the item is deleted
   for (int grid_x = 0; grid_x < _map->getWidth(); grid_x++)
   {
      for (int grid_y = 0; grid_y < _map->getHeight(); grid_y++)
      {
         MapItem* grid_item = _map->getItem(grid_x, grid_y);

         if (grid_item && grid_item->getType() == MapItem::Bomb)
         {
            auto* other_bomb = dynamic_cast<BombMapItem*>(grid_item);

            if (other_bomb && _destroyed_map_items.contains(other_bomb->getShadowedItem()))
            {
               other_bomb->setShadowedItem(nullptr);
            }
         }
      }
   }

   // cleanup map when all explosions are finished
   for (MapItem* destroyed_item : _destroyed_map_items)
   {
      _map->setItem(destroyed_item->getX(), destroyed_item->getY(), nullptr);

      if (destroyed_item->getType() == MapItem::Stone)
      {
         makeFieldImmune(destroyed_item->getX(), destroyed_item->getY());

         auto* stone = dynamic_cast<StoneMapItem*>(destroyed_item);
         std::unique_ptr<ExtraMapItem> extra = stone->releaseExtraMapItem();

         if (extra)
         {
            // owned by the map from here on
            ExtraMapItem* released_extra = extra.release();

            _map->setItem(destroyed_item->getX(), destroyed_item->getY(), released_extra);

            // send mapitem
            _outgoing_packets.push_back(std::make_unique<ExtraMapItemCreatedPacket>(released_extra));

            released_extra->initializeStartTime();
         }
      }
   }

   for (MapItem* destroyed_item : _destroyed_map_items)
   {
      delete destroyed_item;
   }

   _destroyed_map_items.clear();

   if (check_game_over)
   {
      // a player died - so check if there are players left in the game
      updateGameoverCondition();
   }

   // play a bomb sample
   float intensity = detonation_up + detonation_down + detonation_left + detonation_right;

   intensity *= 0.03125f;

   _outgoing_packets.push_back(std::make_unique<GameEventPacket>(GameEventPacket::BombExploded, intensity));

   // first the detonation packet
   _outgoing_packets.push_back(std::make_unique<DetonationPacket>(
      x,
      y,
      static_cast<int8_t>(detonation_up),
      static_cast<int8_t>(detonation_down),
      static_cast<int8_t>(detonation_left),
      static_cast<int8_t>(detonation_right),
      static_cast<float>(flame_count)
   ));

   // remove bombmapitem
   _outgoing_packets.push_back(std::make_unique<MapItemRemovedPacket>(bomb));

   // remove bomb from the map
   _map->setItem(bomb->getX(), bomb->getY(), nullptr);

   // this runs synchronously from inside bomb's own explodedSignal dispatch - destroying it here
   // would destroy the object while one of its own methods is still on the call stack. Defer to
   // the next tick instead, same pattern as BombKickAnimation::explodeDelayed().
   Timer::singleShot(0, [bomb]() { delete bomb; });

   // then the "item destroyed" packets
   std::ranges::move(remove_items, std::back_inserter(_outgoing_packets));
}

bool Game::isGamePopulatedByBots() const
{
   return std::ranges::none_of(
      _players | std::views::values, [](const Player* current_player) { return !current_player->isBot() && !current_player->isKilled(); }
   );
}

void Game::updateGameoverCondition()
{
   int alive_player_count = 0;
   Player* potential_winner = nullptr;

   // check for players that are eventually killed
   for (Player* current_player : _players | std::views::values)
   {
      if (!current_player->isKilled())
      {
         potential_winner = current_player;
         alive_player_count++;
      }
   }

   // the game is over
   if (alive_player_count <= 1)
   {
      processPlayerWon(potential_winner);
      finishGame();
   }

   // only bots left
   else if (isGamePopulatedByBots())
   {
      processOnlyBotsLeft();
   }
}

void Game::resetRoundStats()
{
   for (Player* player : _players | std::views::values)
   {
      player->getRoundStats()->reset();
   }
}

void Game::prepareGame()
{
   // tell clients to clear their maps
   initMapRelatedItems();

   // start countdown
   if (_skip_countdown)
   {
      startGame();
   }
   else
   {
      // update game state
      setState(Constants::GamePreparing);

      // start preparation timer
      _preparation_time.restart();
      _preparation_timer.start();

      _preparation_counter = SERVER_PREPARATION_TIME + SERVER_PREPARATION_SYNC_TIME;

      // first countdown is called immediately
      updatePrepareGame();
   }
}

void Game::updatePrepareGame()
{
   _preparation_counter--;

   _outgoing_packets.push_back(std::make_unique<CountdownPacket>(_preparation_counter));

   if (_preparation_counter == 0)
   {
      qDebug("Game::updatePrepareGame: starting game now");

      _preparation_timer.stop();

      startGame();
   }
   else
   {
      qDebug("Game::updatePrepareGame: starting game in %d secs", _preparation_counter);
   }
}

void Game::finishGame()
{
   if (getState() != Constants::GameFinishing)
   {
      setState(Constants::GameFinishing);

      _map->stopBombs();

      // this is the time to display some sort of finish animation
      singleShotWhileAlive(SERVER_FINISHING_TIME, [this]() { stopGame(); });
   }
}

void Game::stopGame()
{
   setState(Constants::GameStopped);
   setStartPositionsInitialized(false);

   bool finished = false;
   _running = false;

   qDebug("Game::updateGameoverCondition: game over");

   // when someone left the game it could be the case that there's only
   // one player left in the game; for that purpose the server triggers
   // updateGameoverCondition() which detects there's only one player alive
   // therefore the round is over. but that is not sufficient:
   // we don't want to start any more new rounds with only one player left
   // => end the game in that case
   if (getPlayerCount() <= 1 && getPlayersLeftTheGameCount() > 0)
   {
      finished = true;
   }
   else
   {
      // process next round
      nextRound();
      finished = _game_round.isFinished();
   }

   if (finished)
   {
      // send and rounds stats if a round is finished
      broadcastGameStats();
      resetRoundStats();

      // reset count and finished flags, we're done.
      _game_round.reset();
   }

   // send updated game information to everyone (this is mainly done to
   // communicate the updated round count)
   broadcastGameInformation();

   // broadcast "gameover" to all players
   // the "finished" flag is added to let the client know whether to switch
   // back to the lounge or to stay in the win drawable
   _outgoing_packets.push_back(std::make_unique<StopGameResponsePacket>(_game_id, finished));
}

void Game::nextRound()
{
   _game_round.next();

   // restart game until all rounds are finished
   if (!_game_round.isFinished())
   {
      const int delay = SHOW_WINNER_DISPLAY_TIME + SHOW_WINNER_FADE_IN_TIME + SHOW_WINNER_FADE_OUT_TIME + SHOW_WINNER_ADDITIONAL_TIME;

      singleShotWhileAlive(delay, [this]() { prepareGame(); });
   }
}

void Game::startSynchronization()
{
   if (!isSynchronizationActive())
   {
      _synchronization_time.restart();
      setSynchronizationActive(true);

      synchronize();
   }
}

void Game::setSynchronizationActive(bool active)
{
   _synchronization_active = active;
}

bool Game::isSynchronizationActive() const
{
   return _synchronization_active;
}

void Game::synchronize()
{
   // check is all players already sent their "sync" flag
   const std::vector<Player*> players = getPlayers();
   std::vector<Player*> critical_players;
   int bot_count = 0;

   for (Player* player : players)
   {
      if (player->isBot())
      {
         bot_count++;
      }

      if (!player->isLoadingSynchronized())
      {
         critical_players.push_back(player);
      }
   }

   if (critical_players.empty())
   {
      setSynchronizationActive(false);
      prepareGame();
   }
   else if (_synchronization_time.elapsed() < _sync_max_time)
   {
      // otherwise wait and retry
      singleShotWhileAlive(100, [this]() { synchronize(); });
   }
   else
   {
      setSynchronizationActive(false);

      // time has elapsed, kick those who died trying
      for (Player* player : critical_players)
      {
         qDebug("Game::synchronize: time to kick '%s' out of the game", player->getNick().c_str());

         NET_StreamSocket* socket = getSocket(player);
         forceLeaveGameSignal(socket);

         sendPacket(socket, std::make_unique<ErrorPacket>(Constants::ErrorSyncTimeout, "sync aborted.;your pc is too slow."));
      }

      const int players_left_count = static_cast<int>(players.size()) - static_cast<int>(critical_players.size());
      const int human_players_left_count = players_left_count - bot_count;

      // let the others play (if they're more than 2 guys and not only bots)
      if (players_left_count > 1 && players_left_count > bot_count)
      {
         prepareGame();
      }
      else if (human_players_left_count > 0)
      {
         // there's no use running a game with one player in it
         // therefore we stop it.
         stopGame();
      }
   }
}

void Game::sendPacket(NET_StreamSocket* socket, std::unique_ptr<Packet> packet)
{
   packet->serialize();

   if (socket)
   {
      NET_WriteToStreamSocket(socket, packet->constData(), static_cast<int>(packet->size()));
   }
}

void Game::sendBroadcastPackets()
{
   if (_outgoing_packets.empty())
   {
      return;
   }

   // serialize each packet exactly once - Packet::serialize() appends to the packet's own byte
   // buffer rather than resetting it, so serializing once per socket would duplicate payloads
   for (const auto& packet : _outgoing_packets)
   {
      packet->serialize();
   }

   for (const auto& [socket, socket_player] : _player_sockets)
   {
      if (!socket)
      {
         continue;
      }

      const bool synced = socket_player->isLoadingSynchronized();

      // write outgoing packets to socket
      for (const auto& packet : _outgoing_packets)
      {
         const Packet::TYPE packet_type = packet->getType();

         if (synced || packet_type == Packet::JOINGAMERESPONSE || packet_type == Packet::MESSAGE)
         {
            NET_WriteToStreamSocket(socket, packet->constData(), static_cast<int>(packet->size()));
         }
      }
   }

   _outgoing_packets.clear();
}

Player* Game::findPlayer(NET_StreamSocket* socket) const
{
   const auto player_iterator = _player_sockets.find(socket);
   return player_iterator != _player_sockets.end() ? player_iterator->second : nullptr;
}

void Game::processPacket(NET_StreamSocket* tcp_socket, Packet* packet)
{
   switch (packet->getType())
   {
      case Packet::MESSAGE:
      {
         auto* sender_packet = dynamic_cast<MessagePacket*>(packet);

         // during the game we don't care if any of the players is currently
         // typing something. this is just relevant for the lounge
         const bool broadcast_allowed = (_running && sender_packet->isTypingFinished()) || !_running;

         Player* sender = findPlayer(tcp_socket);

         if (broadcast_allowed && sender)
         {
            const int sender_id = sender->getId();

            for (const auto& [current_socket, current_player] : _player_sockets)
            {
               // send message to all players unless it's private
               if (sender_packet->getReceiverId() == -1 || sender_packet->getReceiverId() == current_player->getId())
               {
                  sendPacket(
                     current_socket,
                     std::make_unique<MessagePacket>(
                        sender_id,
                        std::format("{}: {}", sender->getNick(), StringUtils::trim(sender_packet->getMessage())),
                        sender_packet->isTypingFinished(),
                        sender_packet->getReceiverId()
                     )
                  );
               }
            }
         }

         break;
      }

      case Packet::KEY:
      {
         auto* key_packet = dynamic_cast<KeyPacket*>(packet);

         Player* player = findPlayer(tcp_socket);

         if (player && !player->isKilled() && getState() == Constants::GameActive)
         {
            // fix up the player's inputs to not get confused in
            // any way (the player may to go to the left and to
            // the right at the same time for example)
            int8_t keys = key_packet->getKeys();
            const auto previous_keys = static_cast<int8_t>(player->getKeysPressed());

            if ((previous_keys & Constants::KeyRight) && (keys & Constants::KeyLeft))
            {
               keys &= ~(Constants::KeyRight);
            }

            if ((previous_keys & Constants::KeyLeft) && (keys & Constants::KeyRight))
            {
               keys &= ~(Constants::KeyLeft);
            }

            if ((previous_keys & Constants::KeyUp) && (keys & Constants::KeyDown))
            {
               keys &= ~(Constants::KeyUp);
            }

            if ((previous_keys & Constants::KeyDown) && (keys & Constants::KeyUp))
            {
               keys &= ~(Constants::KeyDown);
            }

            if (player->isInfected())
            {
               switch (player->getDisease()->getType())
               {
                  case Constants::SkullAutofire:
                     player->getDisease()->applyAutofire(keys);
                     break;

                  case Constants::SkullKeyboardInvert:
                     player->getDisease()->applyKeyboardInvert(keys);
                     break;

                  default:
                     break;
               }
            }

            /*
               the all-time-famous bomb hickup issue

               client perspective

                  player presses bomb key
                  -> client sends key packet
                  player resets bomb key immediately
                  -> client send key packet

               server perspective

                  one processing cycle:

                     server receives a bomb key packet
                     -> set keyspressed
                     server receives a key packet without the bomb key set
                     -> set keyspressed

                     => bomb keys not noticed :(

               "long story short" => lock the bomb key until it is processed!
            */

            if (keys & Constants::KeyBomb)
            {
               player->setBombKeyLocked(true);
            }

            player->setKeysPressed(keys);
         }

         break;
      }

      case Packet::POSITION:
      {
         qWarning("Game::data(): Packet::POSITION received. no no no. wrong direction!");
         break;
      }

      case Packet::STOPGAMEREQUEST:
      {
         qDebug("Game::data: Packet::STOPGAMEREQUEST received");

         if (getState() == Constants::GameActive)
         {
            Player* player = findPlayer(tcp_socket);

            if (player && getCreator() == player)
            {
               finishGame();

               // show player killed message
               broadcastMessage(std::format("{} aborted the game.", player->getNick()));
            }
         }

         break;
      }

      case Packet::INVALID:
      {
         qWarning("Game::data: Packet::INVALID received");
         break;
      }

      default:
         break;
   }
}

int Game::getId() const
{
   return _game_id;
}

const std::string& Game::getName() const
{
   return _create_game_data._name;
}

const std::string& Game::getLevelName() const
{
   return _create_game_data._level;
}

int Game::getPlayerCount() const
{
   return static_cast<int>(_players.size());
}

int Game::getMaximumPlayerCount() const
{
   return _map->getMaxPlayers();
}

void Game::setCreateGameData(const CreateGameData& data)
{
   _create_game_data = data;
}

void Game::setName(const std::string& name)
{
   _create_game_data._name = name;
}

bool Game::joinGame(Player* player, NET_StreamSocket* player_socket)
{
   if (getPlayerCount() >= getMaximumPlayerCount())
   {
      // game is full
      sendPacket(
         player_socket, std::make_unique<JoinGameResponsePacket>(false, _game_id, player->getId(), player->getNick(), player->getColor())
      );

      return false;
   }

   // assign player color
   player->setColor(getColorForNextPlayer());

   // reset player stats on join game event
   player->resetStats();
   player->setKilled(true);

   // send join information to all other players
   for (Player* existing_player : _players | std::views::values)
   {
      sendPacket(
         player_socket,
         std::make_unique<JoinGameResponsePacket>(
            true, _game_id, existing_player->getId(), existing_player->getNick(), existing_player->getColor()
         )
      );
   }

   // insert the new player and send him his granted packet
   _players[player->getId()] = player;

   _player_sockets[player_socket] = player;

   _outgoing_packets.push_back(
      std::make_unique<JoinGameResponsePacket>(true, _game_id, player->getId(), player->getNick(), player->getColor())
   );

   return true;
}

void Game::processSpectator(NET_StreamSocket* tcp_socket)
{
   if (getState() == Constants::GameActive || getState() == Constants::GamePreparing)
   {
      if (getTimeLeft() > SERVER_MAX_REMAINING_TIME)
      {
         _spectators.push_back(tcp_socket);

         singleShotWhileAlive(SERVER_SPECTATOR_DELAY, [this]() { processSpectatorMessage(); });
      }
   }
}

void Game::processSpectatorMessage()
{
   if (_spectators.empty())
   {
      return;
   }

   NET_StreamSocket* tcp_socket = _spectators.front();
   _spectators.pop_front();

   if (tcp_socket)
   {
      const int time_left = getTimeLeft();

      // tell player next round will start in n seconds
      sendPacket(tcp_socket, std::make_unique<MessagePacket>(-1, "Please wait, the game is currently active.", true));

      sendPacket(
         tcp_socket,
         std::make_unique<MessagePacket>(
            -1,
            (time_left == 1) ? std::string("The next game will start in about 1 second.")
                             : std::format("The next game will start in about {} seconds.", time_left),
            true
         )
      );
   }
}

int Game::getPositionSkipCount() const
{
   return _position_skip_count;
}

int Game::getBotCount() const
{
   return static_cast<int>(std::ranges::count_if(_players | std::views::values, [](const Player* player) { return player->isBot(); }));
}

GameRound* Game::getGameRound()
{
   return &_game_round;
}

void Game::increasePlayersLeftTheGameCount()
{
   _player_left_the_game_count++;
}

void Game::resetPlayersLeftTheGameCount()
{
   _player_left_the_game_count = 0;
}

int Game::getPlayersLeftTheGameCount() const
{
   return _player_left_the_game_count;
}

bool Game::isStartPositionInitialized() const
{
   return _start_positions_initialized;
}

void Game::setStartPositionsInitialized(bool value)
{
   _start_positions_initialized = value;
}

void Game::removePlayer(Player* player, NET_StreamSocket* player_socket)
{
   increasePlayersLeftTheGameCount();

   _player_sockets.erase(player_socket);
   _players.erase(player->getId());

   // drop the socket from the spectator queue too, so processSpectatorMessage()'s
   // delayed timer never fires against a socket that's since been destroyed
   std::erase(_spectators, player_socket);

   // reset player stats on leave game event
   player->resetStats();

   // also the player needs to synchronize loading
   // the next time he's joining a game
   player->setLoadingSynchronized(false);

   playerLeavesSignal(player->getId());
}

void Game::addOutgoingPacket(std::unique_ptr<Packet> packet)
{
   _outgoing_packets.push_back(std::move(packet));
}

int Game::getTimeLeft()
{
   return static_cast<int32_t>(_create_game_data._duration - (_game_time.elapsed() * 0.001f));
}

void Game::processGameTime()
{
   if (getState() == Constants::GameActive)
   {
      const int time_left = getTimeLeft();

      if (time_left <= 0)
      {
         finishGame();
      }

      _outgoing_packets.push_back(std::make_unique<TimePacket>(time_left));
   }
}

void Game::setCreator(Player* creator)
{
   _creator = creator;
}

Player* Game::getCreator() const
{
   return _creator;
}

std::vector<Player*> Game::getPlayers() const
{
   std::vector<Player*> players;
   players.reserve(_players.size());

   for (Player* player : _players | std::views::values)
   {
      players.push_back(player);
   }

   return players;
}

void Game::setState(Constants::GameState state)
{
   _state = state;
   stateChangedSignal(state);
}

Constants::GameState Game::getState() const
{
   return _state;
}

void Game::increaseGamesPlayed()
{
   _games_played++;
}

int Game::getGamesPlayed() const
{
   return _games_played;
}

Constants::Color Game::getColorForNextPlayer() const
{
   // lowest color id (1..10) not taken yet
   for (int i = 1; i <= 10; i++)
   {
      const auto color = static_cast<Constants::Color>(i);

      if (std::ranges::none_of(_players | std::views::values, [color](const Player* player) { return player->getColor() == color; }))
      {
         return color;
      }
   }

   return Constants::ColorWhite;
}

const std::map<NET_StreamSocket*, Player*>& Game::getPlayerSockets() const
{
   return _player_sockets;
}

NET_StreamSocket* Game::getSocket(Player* player) const
{
   const auto socket_iterator = std::ranges::find_if(_player_sockets, [player](const auto& entry) { return entry.second == player; });
   return socket_iterator != _player_sockets.end() ? socket_iterator->first : nullptr;
}

Map* Game::getMap() const
{
   return _map.get();
}

Constants::Dimension Game::getMapDimension() const
{
   return _create_game_data._dimension;
}

void Game::broadcastMessage(const std::string& message)
{
   _outgoing_packets.push_back(std::make_unique<MessagePacket>(-1, message, true));
}

int Game::getExtras() const
{
   int extras = 0;

   if (_create_game_data._extra_bomb_enabled)
   {
      extras |= Constants::ExtraBomb;
   }

   if (_create_game_data._extra_flame_enabled)
   {
      extras |= Constants::ExtraFlame;
   }

   if (_create_game_data._extra_speedup_enabled)
   {
      extras |= Constants::ExtraSpeedup;
   }

   if (_create_game_data._extra_kick_enabled)
   {
      extras |= Constants::ExtraKick;
   }

   if (_create_game_data._extra_skulls_enabled)
   {
      extras |= Constants::ExtraSkull;
   }

   return extras;
}

int Game::getDuration() const
{
   return _create_game_data._duration;
}

void Game::bombKickedAnimation(BombMapItem* item, Constants::Direction direction, float speed)
{
   if (item)
   {
      _outgoing_packets.push_back(std::make_unique<MapItemMovePacket>(item->getUniqueId(), speed, direction, item->getX(), item->getY()));
   }
}

void Game::initializeExtraSpawn()
{
   _extra_spawn = std::make_unique<ExtraSpawn>();
   _extra_spawn->setMap(getMap());

   _extra_spawn->spawnSignal.connect([this]() { spawn(); });
}

void Game::spawn()
{
   // only spawn extras in active state
   if (getState() != Constants::GameActive)
   {
      return;
   }

   Map* map = getMap();
   const int width = map->getWidth();
   const int height = map->getHeight();

   bool done = false;

   while (!done)
   {
      const int x = Random::bounded(width);
      const int y = Random::bounded(height);

      // there must be no item and no living player at x,y
      if (!map->getItem(x, y))
      {
         done = std::ranges::none_of(
            _players | std::views::values,
            [x, y](const Player* player)
            {
               return !player->isKilled() && static_cast<int32_t>(std::floor(player->getX())) == x &&
                      static_cast<int32_t>(std::floor(player->getY())) == y;
            }
         );
      }
   }
}

bool Game::isSpawnExtrasEnabled() const
{
   return _extra_spawn_enabled;
}
