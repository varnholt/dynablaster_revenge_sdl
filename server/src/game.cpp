// header
#include "game.h"
#include <cstring>

// shared
#include "bombkickanimation.h"
#include "bombmapitem.h"
#include "bombpacket.h"
#include "collisiondetection.h"
#include "countdownpacket.h"
#include "detonationpacket.h"
#include "errorpacket.h"
#include "extramapitem.h"
#include "extramapitemcreatedpacket.h"
#include "extrashakepackethandler.h"
#include "extraspawn.h"
#include "gameeventpacket.h"
#include "gamestatspacket.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "keypacket.h"
#include "listgamesresponsepacket.h"
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
#include "startgamerequestpacket.h"
#include "startgameresponsepacket.h"
#include "stonemapitem.h"
#include "stopgamerequestpacket.h"
#include "stopgameresponsepacket.h"
#include "stringutils.h"
#include "timepacket.h"

// Qt
#include "logging.h"

#include "random.h"
#include "settings.h"

// stdlib
#include <algorithm>
#include <format>
#include <memory>
#include <numbers>
#include <unordered_set>
#include <vector>

// SDL
#include <SDL3_net/SDL_net.h>

// c
#include <cmath>
#include <cstdint>

// static variables
int Game::sGameId = 0;

//-----------------------------------------------------------------------------
/*!
   constructor
*/
Game::Game()
    : mMap(nullptr),
      mGameId(++sGameId),
      mRunning(false),
      mIdlePacketSent(false),
      mDuration(0),
      mCreator(nullptr),
      mState(Constants::GameStopped),
      mPreparationCounter(0),
      mCheckGameOver(false),
      mSkipCountdown(false),
      mPositionSkipCount(0),
      mGamesPlayed(0),
      mSynchronizationActive(false),
      mMaxSpeed(0.0),
      mSyncMaxTime(0),
      mCollisionDetection(nullptr),
      mImmuneTimes(nullptr),
      mPlayerLeftTheGameCount(0),
      mStartPositionsInitialized(false),
      mGameOnlyPopulatedByBotsMessageShown(false),
      mExtraSpawn(nullptr),
      mExtraSpawnEnabled(false)
{
   qDebug("Game::Game: initializing");

   // init config
   Settings settings(SERVER_CONFIG_FILE_SERVER, Settings::IniFormat);

   mPositionSkipCount = settings.value("skip_positions", 1).toInt();
   mSkipCountdown = settings.value("skip_countdown", false).toBool();
   BombMapItem::setTickTime(settings.value("tick_count", SERVER_BOMB_TICKTIME_DEFAULT).toInt());
   bool shakePacketsEnabled = settings.value("shake_packets", true).toBool();

   mMaxSpeed = SERVER_DEFAULT_SPEED + (SERVER_SPEEDUP_INCREMENT * SERVER_MAX_SPEEDUPS);

   mSyncMaxTime = settings.value("player_sync_max_time", SERVER_PLAYER_SYNC_MAX_TIME).toInt();

   // init direction maps
   mDirectionCheckCenter.push_back(Constants::DirectionUp);
   mDirectionCheckAll.push_back(Constants::DirectionUp);
   mDirectionCheckAll.push_back(Constants::DirectionDown);
   mDirectionCheckAll.push_back(Constants::DirectionLeft);
   mDirectionCheckAll.push_back(Constants::DirectionRight);

   // shake packet handler
   mShakePacketHandler = std::make_unique<ExtraShakePacketHandler>();
   mShakePacketHandler->setGame(this);
   mShakePacketHandler->setEnabled(shakePacketsEnabled);

   // collision detection
   mCollisionDetection = std::make_unique<CollisionDetection>();
   mCollisionDetection->setGame(this);

   mCollisionDetection->playerKicksBombSignal.connect([this](Player* player, MapItem* item, bool verticallyKicked, int keysPressed)
                                                      { playerKicksBomb(player, item, verticallyKicked, keysPressed); });

   mCollisionDetection->playerIdleSignal.connect([this](int8_t directions, Player* player) { playerIdle(directions, player); });

   mCollisionDetection->playerMoveSignal.connect([this](Player* player, float assignedXPos, float assignedYPos, int8_t directions)
                                                 { playerMove(player, assignedXPos, assignedYPos, directions); });

   // init skulls
   initSkullSetup();
}

//-----------------------------------------------------------------------------
/*!
   destructor
*/
Game::~Game()
{
   qDebug("Game::~Game");

   // remove those animations first since they access mMap
   BombKickAnimation::deleteAll();

   delete mMap;
   delete mImmuneTimes;

   mMap = nullptr;
   mImmuneTimes = nullptr;
}

//-----------------------------------------------------------------------------
/*!
   initialize server
*/
void Game::initSkullSetup()
{
   // i disabled reading the skulls from configuration for now..
   // from my point of view it just creates unnecessary confusion

   // init config
   /*
   QSettings settings (
      SERVER_CONFIG_FILE_SERVER,
      QSettings::IniFormat
   );
   */

   // supported skulls

   // used skulls
   bool skullAutofire = true;        // settings.value("skull_autofire", true).toBool();
   bool skullMinimumBomb = true;     // settings.value("skull_minimum_bomb", true).toBool();
   bool skullKeyboardInvert = true;  // settings.value("skull_keyboard_invert", true).toBool();
   bool skullMushroom = true;        // settings.value("skull_mushroom", true).toBool();
   bool skullInvisible = true;       // settings.value("skull_invisible", true).toBool();
   bool skullInvincible = true;      // settings.value("skull_invincible", true).toBool();

   // unused for now
   bool skullMaximumBomb = false;  // settings.value("skull_maximum_bomb", false).toBool();
   bool skullSlow = false;         // settings.value("skull_slow", false).toBool();
   bool skullFast = false;         // settings.value("skull_fast", false).toBool();
   bool skullNoBomb = false;       // settings.value("skull_nobomb", false).toBool();

   // skull face ordering

   /*
      [0] red    -> autofire
      [1] blue   -> min bomb
      [2] purple -> keyboardinvert
      [3] dizzy  -> mushroom
      [4] green  -> invisible
      [5] gold   -> invincible
   */

   // used sides
   int skullAutofireSide = 0;        // settings.value("skull_autofire_side", 0).toInt();
   int skullMinimumBombSide = 1;     // settings.value("skull_minimum_bomb_side", 1).toInt();
   int skullKeyboardInvertSide = 2;  // settings.value("skull_keyboard_invert_side", 2).toInt();
   int skullMushroomSide = 3;        // settings.value("skull_mushroom_side", 3).toInt();
   int skullInvisibleSide = 4;       // settings.value("skull_invisible_side", 4).toInt();
   int skullInvincibleSide = 5;      // settings.value("skull_invincible_side", 5).toInt();

   // unused for now
   int skullMaximumBombSide = -1;  // settings.value("skull_maximum_bomb_side", -1).toInt();
   int skullSlowSide = -1;         // settings.value("skull_slow_side", -1).toInt();
   int skullFastSide = -1;         // settings.value("skull_fast_side", -1).toInt();
   int skullNoBombSide = -1;       // settings.value("skull_nobomb_side", -1).toInt();

   std::vector<Constants::SkullType> faces;
   for (int i = 0; i < 6; i++)
      faces.push_back(Constants::SkullReset);

   if (skullAutofireSide > -1 && skullAutofireSide < 6)
      faces[skullAutofireSide] = Constants::SkullAutofire;

   if (skullKeyboardInvertSide > -1 && skullKeyboardInvertSide < 6)
      faces[skullKeyboardInvertSide] = Constants::SkullKeyboardInvert;

   if (skullMushroomSide > -1 && skullMushroomSide < 6)
      faces[skullMushroomSide] = Constants::SkullMushroom;

   if (skullInvisibleSide > -1 && skullInvisibleSide < 6)
      faces[skullInvisibleSide] = Constants::SkullInvisible;

   if (skullInvincibleSide > -1 && skullInvincibleSide < 6)
      faces[skullInvincibleSide] = Constants::SkullInvincible;

   if (skullMinimumBombSide > -1 && skullMinimumBombSide < 6)
      faces[skullMinimumBombSide] = Constants::SkullMinimumBomb;

   if (skullMaximumBombSide > -1 && skullMaximumBombSide < 6)
      faces[skullMaximumBombSide] = Constants::SkullMaximumBomb;

   if (skullSlowSide > -1 && skullSlowSide < 6)
      faces[skullSlowSide] = Constants::SkullSlow;

   if (skullFastSide > -1 && skullFastSide < 6)
      faces[skullFastSide] = Constants::SkullFast;

   if (skullNoBombSide > -1 && skullNoBombSide < 6)
      faces[skullNoBombSide] = Constants::SkullNoBomb;

   PlayerDisease::setSkullFaces(faces);

   // init set of supported skulls
   std::unordered_set<Constants::SkullType> supportedSkulls;

   if (skullAutofire)
      supportedSkulls.insert(Constants::SkullAutofire);

   if (skullKeyboardInvert)
      supportedSkulls.insert(Constants::SkullKeyboardInvert);

   if (skullMushroom)
      supportedSkulls.insert(Constants::SkullMushroom);

   if (skullInvisible)
      supportedSkulls.insert(Constants::SkullInvisible);

   if (skullInvincible)
      supportedSkulls.insert(Constants::SkullInvincible);

   if (skullMinimumBomb)
      supportedSkulls.insert(Constants::SkullMinimumBomb);

   if (skullMaximumBomb)
      supportedSkulls.insert(Constants::SkullMaximumBomb);

   if (skullSlow)
      supportedSkulls.insert(Constants::SkullSlow);

   if (skullFast)
      supportedSkulls.insert(Constants::SkullFast);

   if (skullNoBomb)
      supportedSkulls.insert(Constants::SkullNoBomb);

   PlayerDisease::setSupportedSkulls(supportedSkulls);
}

//-----------------------------------------------------------------------------
/*!
   initialize server
*/
void Game::initialize()
{
   qDebug("Game::initialize");

   initializeMap();
   initializeTimers();
}

//-----------------------------------------------------------------------------
/*!
   initialize map
*/
void Game::initializeMap()
{
   // if there are still items in the destroyed map, clear that map.
   // that map is only processed in 'bombExploded()' which is only triggered
   // when the game is not stopped. therefore the may be items left in this
   // map after one round
   mDestroyedMapItems.clear();

   // if there was a map before, delete it
   delete mMap;

   // init
   int width = 0;
   int height = 0;
   int stones = 0;

   float extraCount = 0.0f;
   int extraBombs = static_cast<int32_t>(mCreateGameData.mExtraBombEnabled);
   int extraFlames = static_cast<int32_t>(mCreateGameData.mExtraFlameEnabled);
   int extraKicks = static_cast<int32_t>(mCreateGameData.mExtraKickEnabled);
   int extraSpeedUps = static_cast<int32_t>(mCreateGameData.mExtraSpeedupEnabled);
   int extraSkulls = static_cast<int32_t>(mCreateGameData.mExtraSkullsEnabled);

   float extraSum = (extraBombs * SERVER_WEIGHT_BOMBS) + (extraFlames * SERVER_WEIGHT_FLAMES) + (extraKicks * SERVER_WEIGHT_KICKS) +
                    (extraSpeedUps * SERVER_WEIGHT_SPEEDUPS) + (extraSkulls * SERVER_WEIGHT_SKULLS);

   // 13x11 field: 16 extras
   // 26x22 field: 32 extras

   // create default map
   std::vector<Point> startPositions;

   if (mCreateGameData.mDimension == Constants::Dimension13x11)
   {
      extraCount = 16.0f;

      width = 13;
      height = 11;
      stones = 60;

      startPositions = {Point(0, 0), Point(12, 10), Point(12, 0), Point(0, 10), Point(6, 5)};
   }

   else if (mCreateGameData.mDimension == Constants::Dimension19x17)
   {
      extraCount = 32.0f;

      width = 19;
      height = 17;
      stones = 120;

      startPositions = {
         Point(0, 0),
         Point(width - 1, height - 1),
         Point(width - 1, 0),
         Point(0, height - 1),
         Point(0, 8),     // center left
         Point(18, 8),    // center right
         Point(6, 4),     // center quad top left
         Point(12, 4),    // center quad top right
         Point(6, 12),    // center quad bottom left
         Point(12, 12),   // center quad bottom right
      };
   }

   else if (mCreateGameData.mDimension == Constants::Dimension25x21)
   {
      extraCount = 32.0f;

      width = 25;
      height = 21;
      stones = 140;

      startPositions = {
         Point(0, 0),
         Point(width - 1, height - 1),
         Point(width - 1, 0),
         Point(0, height - 1),
         Point(12, 0),    // center top
         Point(12, 20),   // center bottom
         Point(7, 6),     // center quad top left
         Point(17, 6),    // center quad top right
         Point(7, 14),    // center quad bottom left
         Point(17, 14),   // center quad bottom right
      };
   }

   float valBombs = 0.0f;
   float valFlames = 0.0f;
   float valKicks = 0.0f;
   float valSpeedUps = 0.0f;
   float valSkulls = 0.0f;

   if (extraBombs > 0)
      valBombs = extraCount * (SERVER_WEIGHT_BOMBS / extraSum);

   if (extraFlames > 0)
      valFlames = extraCount * (SERVER_WEIGHT_FLAMES / extraSum);

   if (extraKicks > 0)
      valKicks = extraCount * (SERVER_WEIGHT_KICKS / extraSum);

   if (extraSpeedUps > 0)
      valSpeedUps = extraCount * (SERVER_WEIGHT_SPEEDUPS / extraSum);

   if (extraSkulls > 0)
      valSkulls = extraCount * (SERVER_WEIGHT_SKULLS / extraSum);

   extraBombs = static_cast<int32_t>(valBombs);
   extraFlames = static_cast<int32_t>(valFlames);
   extraKicks = static_cast<int32_t>(valKicks);
   extraSpeedUps = static_cast<int32_t>(valSpeedUps);
   extraSkulls = static_cast<int32_t>(valSkulls);

   int loop = 0;
   while ((extraBombs + extraFlames + extraKicks + extraSpeedUps + extraSkulls) < extraCount && loop < 5)
   {
      if (loop == 0)
         extraBombs = static_cast<int32_t>(std::ceil(valBombs));
      if (loop == 1)
         extraFlames = static_cast<int32_t>(std::ceil(valFlames));
      if (loop == 2)
         extraKicks = static_cast<int32_t>(std::ceil(valKicks));
      if (loop == 3)
         extraSpeedUps = static_cast<int32_t>(std::ceil(valSpeedUps));
      if (loop == 4)
         extraSkulls = static_cast<int32_t>(std::ceil(valSkulls));

      loop++;
   }

   mMap = Map::generateMap(
      width,          // width
      height,         // height
      stones,         // stones
      extraBombs,     // bombs
      extraFlames,    // flames
      extraSpeedUps,  // speedups
      extraKicks,     // kicks
      extraSkulls,    // skulls
      startPositions  // start positions
   );

   delete mImmuneTimes;
   uint32_t fieldCount = static_cast<uint32_t>(width * height);
   mImmuneTimes = new int[fieldCount];
   std::memset(mImmuneTimes, 0, fieldCount * sizeof(int));
}

//-----------------------------------------------------------------------------
/*!
   initialize timers
*/
void Game::initializeTimers()
{
   qDebug("Game::initializeTimers");

   // update timer
   mUpdateTimer.timeoutSignal.connect([this]() { update(); });
   mUpdateTimer.start(1000 / SERVER_HEARTBEAT_IN_HZ);

   // game time timer
   mGameTimeUpdateTimer.timeoutSignal.connect([this]() { processGameTime(); });
   mGameTimeUpdateTimer.start(1000);

   // game preparation timer
   mPreparationTimer.setInterval(1000);
   mPreparationTimer.timeoutSignal.connect([this]() { updatePrepareGame(); });
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::broadcastStartPositions()
{
   for (const auto& [id, currentPlayer] : mPlayers)
   {
      PositionPacket* positionPacket = new PositionPacket(
         currentPlayer->getId(), Constants::KeyDown, currentPlayer->getX(), currentPlayer->getY(), std::numbers::pi_v<float> * 1.5f
      );

      mOutgoingPackets.push_back(positionPacket);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::initializePlayerStartPositions()
{
   int startPositionIndex = 0;
   for (const auto& [id, currentPlayer] : mPlayers)
   {
      // reset player's extras
      currentPlayer->reset();
      currentPlayer->getPlayerRotation()->reset();

      // reposition player
      Point startPosition = mMap->getStartPosition(startPositionIndex);
      currentPlayer->setX(startPosition.x() + 0.5f);
      currentPlayer->setY(startPosition.y() + 0.5f);

      startPositionIndex++;
   }

   setStartPositionsInitialized(true);
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::broadcastCreateMapItems()
{
   std::vector<MapItemCreatedPacket*> createPackets = mMap->getMapItemCreatedPackets();
   for (int i = 0; i < static_cast<int>(createPackets.size()); i++)
      mOutgoingPackets.push_back(createPackets[i]);
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::broadcastClearMapItems()
{
   std::vector<MapItemRemovedPacket*> removePackets = mMap->getMapItemRemovedPackets();
   for (int i = 0; i < static_cast<int>(removePackets.size()); i++)
      mOutgoingPackets.push_back(removePackets[i]);
}

//-----------------------------------------------------------------------------
/*!
 */
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

//-----------------------------------------------------------------------------
/*!
 */
void Game::broadcastGameInformation()
{
   std::vector<GameInformation> games;
   games.push_back(getGameInformation());
   mOutgoingPackets.push_back(new ListGamesResponsePacket(games, true));
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::broadcastStartGame()
{
   mOutgoingPackets.push_back(new StartGameResponsePacket(mGameId, true));
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::startGame()
{
   qDebug("Game::startNewGame");

   // reset the number of players that left during the running game
   setGameOnlyPopulatedByBotsMessageShown(false);
   resetPlayersLeftTheGameCount();

   // reset the game time
   mGameTime.restart();

   // set state machine to active
   setState(Constants::GameActive);

   mRunning = true;
   mIdlePacketSent = false;

   // increase the number of rounds played
   increaseGamesPlayed();

   // broadcast game time
   processGameTime();

   // broadcast new game information to all players
   broadcastGameInformation();

   // broadcast "started" to all players
   broadcastStartGame();
}

//-----------------------------------------------------------------------------
/*!
   \return game information object
*/
GameInformation Game::getGameInformation()
{
   GameInformation gameInformation = GameInformation(
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

   return gameInformation;
}

//-----------------------------------------------------------------------------
/*!
   the main update loop
*/
void Game::update()
{
   // update positions
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

//-----------------------------------------------------------------------------
/*!
   update all positions
*/
void Game::updatePositions()
{
   // update player positions
   updatePlayerPositions();
}

//-----------------------------------------------------------------------------
/*!
   update player positions
*/
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
      for (const auto& [socket, player] : mPlayerSockets)
      {
         mCollisionDetection->process(player);
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
bool Game::isKickPossible(int x, int y, Constants::Direction kickDir)
{
   // kick if possible if level dimensions not exceeded and
   // if there's no bomb, stone or block located in the kick
   // direction

   bool kickPossible = false;

   int checkOffsetX = x;
   int checkOffsetY = y;

   switch (kickDir)
   {
      case Constants::DirectionUp:
         checkOffsetY -= 1;
         break;
      case Constants::DirectionDown:
         checkOffsetY += 1;
         break;
      case Constants::DirectionLeft:
         checkOffsetX -= 1;
         break;
      case Constants::DirectionRight:
         checkOffsetX += 1;
         break;
      default:
         break;
   }

   if (checkOffsetX >= 0 && checkOffsetX < getMap()->getWidth() && checkOffsetY >= 0 && checkOffsetY < getMap()->getHeight())
   {
      // check if there is an obstructing map item
      MapItem* item = getMap()->getItem(checkOffsetX, checkOffsetY);

      if (item)
      {
         if (item->getType() == MapItem::Extra)
         {
            kickPossible = true;
         }
      }
      else
      {
         kickPossible = true;
      }

      // check if there is an obstructing player
      if (kickPossible)
      {
         int px = 0;
         int py = 0;

         for (const auto& [id, p] : mPlayers)
         {
            if (!p->isKilled())
            {
               px = static_cast<int32_t>(std::floor(p->getX()));
               py = static_cast<int32_t>(std::floor(p->getY()));

               // there is an obstructing player
               if (px == checkOffsetX && py == checkOffsetY)
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
                     then just set kickPossible = false.
                  */

                  // horizontal kick movement
                  if (kickDir == Constants::DirectionLeft || kickDir == Constants::DirectionRight)
                  {
                     float dx = std::fabs(p->getX() - x);

                     if (dx < 1.0f + SERVER_KICK_PLAYER_DISTANCE)
                     {
                        kickPossible = false;
                     }
                  }

                  // vertical kick movement
                  if (kickDir == Constants::DirectionUp || kickDir == Constants::DirectionDown)
                  {
                     float dy = std::fabs(p->getY() - y);

                     if (dy < 1.0f + SERVER_KICK_PLAYER_DISTANCE)
                     {
                        kickPossible = false;
                     }
                  }
               }
            }
         }
      }
   }

   return kickPossible;
}

//-----------------------------------------------------------------------------
/*!
   \param kickedBomb bomb the kick animation is based on
   \param kickDir kick direction
*/
void Game::createKickAnimation(BombMapItem* kickedBomb, Constants::Direction kickDir)
{
   kickedBomb->kickAnimationSignal.connect(
      [this, kickedBomb](Constants::Direction direction, float speed) { bombKickedAnimation(kickedBomb, direction, speed); }
   );

   // init kick animation
   // also a kick animation needs the playfield map
   // for collision detection
   auto bkaOwner = std::make_unique<BombKickAnimation>();
   BombKickAnimation* bka = bkaOwner.get();
   bka->setDirection(kickDir);
   bka->setMap(getMap());

   // a kick animations needs to "know" about current player
   // positions so they bounce back once they hit another player
   for (const auto& [id, p] : mPlayers)
   {
      if (!p->isKilled())
      {
         bka->updatePlayerPosition(p->getId(), p->getX(), p->getY());
      }
   }

   // bka is short-lived (destroyed once the kick/explosion finishes) but the 3 signals below
   // live for the whole match - unlike Qt's connect(), Signal<> won't auto-disconnect a
   // destroyed subscriber, so bka has to disconnect itself on the way out (see
   // BombKickAnimation::addDestroyCallback()).
   auto positionChangedConnection =
      mCollisionDetection->playerPositionChangedSignal.connect([bka](int id, float x, float y) { bka->updatePlayerPosition(id, x, y); });
   auto playerKilledConnection = playerKilledSignal.connect([bka](int id) { bka->removePlayerPosition(id); });
   auto playerLeavesConnection = playerLeavesSignal.connect([bka](int id) { bka->removePlayerPosition(id); });

   bka->addDestroyCallback(
      [this, positionChangedConnection, playerKilledConnection, playerLeavesConnection]()
      {
         mCollisionDetection->playerPositionChangedSignal.disconnect(positionChangedConnection);
         playerKilledSignal.disconnect(playerKilledConnection);
         playerLeavesSignal.disconnect(playerLeavesConnection);
      }
   );

   kickedBomb->setBombKickAnimation(std::move(bkaOwner));
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::updateImmuneTimes()
{
   int val = 0;
   int pos = 0;
   for (int y = 0; y < mMap->getHeight(); y++)
   {
      for (int x = 0; x < mMap->getWidth(); x++)
      {
         pos = y * mMap->getWidth() + x;
         val = mImmuneTimes[pos];
         val -= (1000 / SERVER_HEARTBEAT_IN_HZ);
         mImmuneTimes[pos] = std::max(0, val);
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
*/
void Game::makeFieldImmune(int x, int y)
{
   mImmuneTimes[y * mMap->getWidth() + x] = SERVER_IMMUNE_FIELD_DELAY;
}

//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \return \c true if field is immune
*/
bool Game::isFieldImmune(int x, int y) const
{
   return mImmuneTimes[y * mMap->getWidth() + x] > 0;
}

//-----------------------------------------------------------------------------
/*!
   \param player affected player
   \param item mapitem
*/
void Game::playerKicksBomb(Player* player, MapItem* item, bool verticallyKicked, int keysPressed)
{
   if (item && item->getType() == MapItem::Bomb && !player->isKilled() && player->isKickEnabled())
   {
      BombMapItem* kickedBomb = dynamic_cast<BombMapItem*>(item);

      if (kickedBomb)
      {
         // init kick direction
         Constants::Direction kickDir = Constants::DirectionUnknown;

         if (verticallyKicked)
         {
            kickDir = (keysPressed & Constants::KeyUp) ? Constants::DirectionUp : Constants::DirectionDown;
         }
         else
         {
            kickDir = (keysPressed & Constants::KeyLeft) ? Constants::DirectionLeft : Constants::DirectionRight;
         }

         if (isKickPossible(item->getX(), item->getY(), kickDir))
         {
            if (!kickedBomb->isKicked())
            {
               createKickAnimation(kickedBomb, kickDir);
            }
            else
            {
               // bomb has already been kicked once - getBombKickAnimation() can be null if the
               // animation already finished/exploded between kicks
               BombKickAnimation* bka = kickedBomb->getBombKickAnimation();

               if (bka)
               {
                  // update kick-direction
                  bka->setDirection(kickDir);
               }
            }

            // kick it
            kickedBomb->kick();
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param disease disease that just stopped
*/
void Game::playerDiseaseStopped(PlayerDisease* disease)
{
   if (disease)
   {
      mOutgoingPackets.push_back(new PlayerInfectedPacket(disease->getPlayerId(), Constants::SkullReset));
   }
}

//-----------------------------------------------------------------------------
/*!
   \param player affected player
   \param assignedXPos assigned x position
   \param assignedYPos assigned y position
   \param directions player directions
*/
void Game::playerMove(Player* player, float assignedXPos, float assignedYPos, int8_t directions)
{
   float speedX = 0.0;
   float speedY = 0.0;

   if (!player->isKilled())
   {
      speedX = assignedXPos - player->getX();
      speedY = assignedYPos - player->getY();
   }

   mOutgoingPackets.push_back(new PositionPacket(
      player->getId(),
      directions,
      assignedXPos,
      assignedYPos,
      player->getPlayerRotation()->getAngle(),
      speedX,
      speedY,
      player->getPlayerRotation()->getAngleDelta(),
      player->getSpeed()
   ));

   player->setPositionSkipCounter(0);
   player->setKeysPressed(player->getKeysPressed());

   // reset idle packet flag
   mIdlePacketSentSet.erase(player);
}

//-----------------------------------------------------------------------------
/*!
   \param directions player's directions
   \param player affected player
*/
void Game::playerIdle(int8_t directions, Player* player)
{
   if (!mIdlePacketSentSet.contains(player))
   {
      // send a zero-distance (speed=0.0 packet)
      mOutgoingPackets.push_back(new PositionPacket(
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

      mIdlePacketSentSet.insert(player);
   }
}

//-----------------------------------------------------------------------------
/*!
   \param player infected player
*/
void Game::createInfection(
   Player* infectedPlayer, Player* infectingPlayer, ExtraMapItem* extra, const std::vector<Constants::SkullType>& faces
)
{
   float extraElapsedTime = 0.0f;

   if (extra)
      extraElapsedTime = extra->getElapsedTime();

   auto disease = std::make_unique<PlayerDisease>();
   PlayerDisease* diseasePtr = disease.get();
   diseasePtr->setPlayerId(infectedPlayer->getId());

   if (infectingPlayer)
   {
      // infecting player infects other player
      diseasePtr->setType(infectingPlayer->getDisease()->getType());
   }
   else
   {
      // find out which side the rotating cube is showing right now
      int sideStep = (static_cast<int32_t>(std::floor(extraElapsedTime + 0.5f))) % 6;
      Constants::SkullType skullType = faces[sideStep];
      diseasePtr->setType(skullType);
   }

   // if the infected player has been infected before, abort the old infection
   if (infectedPlayer->getDisease())
      infectedPlayer->getDisease()->abort();

   diseasePtr->activate();

   // whenever the game changes to 'stopped' state we want to abort the disease and send an
   // according packet to the client - disease is short-lived (destroyed once cured/aborted, or
   // directly by ~Player() on disconnect - see project_full_qt_removal_scope memory),
   // stateChangedSignal lives for the whole match, so disease has to disconnect itself on the
   // way out (Signal<> has no auto-disconnect-on-destroy the way Qt's connect() did).
   auto stateChangedConnection = stateChangedSignal.connect([diseasePtr](Constants::GameState) { diseasePtr->abort(); });
   diseasePtr->addDestroyCallback([this, stateChangedConnection]() { stateChangedSignal.disconnect(stateChangedConnection); });

   diseasePtr->stoppedSignal.connect([this, diseasePtr]() { playerDiseaseStopped(diseasePtr); });

   PlayerInfectedPacket* packet = new PlayerInfectedPacket(infectedPlayer->getId(), diseasePtr->getType());

   infectedPlayer->infect(std::move(disease));

   // either add the id of the player that infected the other
   // or add the position of the extra the extra is contained in
   if (infectingPlayer)
   {
      packet->setInfectorId(infectingPlayer->getId());
   }
   else
   {
      packet->setExtraPos(extra->getX(), extra->getY());
   }

   mOutgoingPackets.push_back(packet);
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::updateExtras()
{
   for (const auto& [socket, player] : mPlayerSockets)
   {
      // init player position
      int x = static_cast<int32_t>(std::floor(player->getX()));
      int y = static_cast<int32_t>(std::floor(player->getY()));

      MapItem* mapItem = mMap->getItem(x, y);

      if (mapItem && mapItem->getType() == MapItem::Extra)
      {
         // update player stats
         player->increaseExtrasCollected();

         // evaluate extra and pass it to the player
         ExtraMapItem* extra = dynamic_cast<ExtraMapItem*>(mapItem);

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
               player->setSpeed(std::min(player->getSpeed() + SERVER_SPEEDUP_INCREMENT, mMaxSpeed));

               break;
            }

            case Constants::ExtraKick:
            {
               player->setKickEnabled(true);
               break;
            }

            case Constants::ExtraSkull:
            {
               std::vector<Constants::SkullType> faces = extra->getSkullFaces();
               createInfection(player, nullptr, extra, faces);
               break;
            }
         }

         // play that lovely coin sample
         GameEventPacket* gameEventPacket = new GameEventPacket(GameEventPacket::ExtraCollected, 1.0f, x, y);

         gameEventPacket->setPlayerId(player->getId());
         gameEventPacket->setExtraType(extra->getExtraType());

         mOutgoingPackets.push_back(gameEventPacket);

         // remove extramapitem
         mOutgoingPackets.push_back(new MapItemRemovedPacket(extra));

         // remove extra from the map
         mMap->setItem(x, y, nullptr);

         delete extra;

         // if the extra is removed here, remove it from the destroyed maps
         // also, so we don't have a dangling pointer in that list
         mDestroyedMapItems.erase(extra);
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::updateInfections()
{
   // it is only required to go through here if skulls are used
   if (mCreateGameData.mExtraSkullsEnabled)
   {
      std::vector<Player*> players = getPlayers();
      Vec2 pos1;
      Vec2 pos2;
      Vec2 vec;

      for (Player* player1 : players)
      {
         if (player1->isInfected() && !player1->isKilled())
         {
            for (Player* player2 : players)
            {
               // player must be someone else
               if (player2 != player1)
               {
                  // player "to be infected" must be not infected yet
                  if (!player2->isInfected() && !player2->isKilled())
                  {
                     pos1 = Vec2(player1->getX(), player1->getY());
                     pos2 = Vec2(player2->getX(), player2->getY());

                     vec = pos1 - pos2;

                     // if they are near enough to each other
                     if (vec.length() < 0.3f)
                     {
                        // player2: the player who is now infected
                        // player1: the one who was already infected
                        createInfection(player2, player1, nullptr);
                     }
                  }
               }
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::updateBombs()
{
   for (const auto& [socket, player] : mPlayerSockets)
   {
      // int keysPressed = player->getKeysPressed();

      // check if player wants to drop a bomb
      if (player->isBombKeyLocked())
      {
         int x = static_cast<int32_t>(std::floor(player->getX()));
         int y = static_cast<int32_t>(std::floor(player->getY()));
         MapItem* item = mMap->getItem(x, y);

         if (!item)
         {
            // if player is allowed to drop more bombs
            if (player->getBombsDroppedCount() < player->getBombCount())
            {
               player->setBombsDroppedCount(player->getBombsDroppedCount() + 1);

               BombMapItem* bomb = new BombMapItem(player->getId(), player->getFlameCount(), -1, x, y);

               bomb->explodedSignal.connect([this](BombMapItem* b, bool recursive) { bombExploded(b, recursive); });

               mMap->setItem(x, y, bomb);

               mOutgoingPackets.push_back(new MapItemCreatedPacket(bomb, player->getId()));
            }
         }

         // remove bomb key once a bomb has been dropped
         player->setKeysPressed(player->getKeysPressed() & ~Constants::KeyBomb);

         player->setBombKeyLocked(false);
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param killer bomb dropper
   \param victim player who was killed
*/
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

//-----------------------------------------------------------------------------
/*!
   \param player player who won
*/
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

//-----------------------------------------------------------------------------
/*!
   \param message message to send
*/
void Game::sendMessageToOwner(const std::string& message)
{
   MessagePacket* messagePacket = new MessagePacket(mCreator->getId(), message, true);

   NET_StreamSocket* socket = nullptr;
   for (const auto& [candidateSocket, candidatePlayer] : mPlayerSockets)
   {
      if (candidatePlayer == mCreator)
      {
         socket = candidateSocket;
         break;
      }
   }

   sendPacket(socket, messagePacket);
}

//-----------------------------------------------------------------------------
/*!
   \return message shown flag
*/
bool Game::isGameOnlyPopulatedByBotsMessageShown() const
{
   return mGameOnlyPopulatedByBotsMessageShown;
}

//-----------------------------------------------------------------------------
/*!
   \param message message shown flag
*/
void Game::setGameOnlyPopulatedByBotsMessageShown(bool value)
{
   mGameOnlyPopulatedByBotsMessageShown = value;
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::processOnlyBotsLeft()
{
   qDebug("Game::processOnlyBotsLeft: the game is now only populated by bots");

   std::string message = "The game is now only populated by bots. Press F10 to abort.";

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

//-----------------------------------------------------------------------------
/*!
 */
void Game::broadcastGameStats()
{
   std::vector<int> ids;
   std::vector<PlayerStats> overallStats;
   std::vector<PlayerStats> roundStats;

   for (const auto& [id, p] : mPlayers)
   {
      ids.push_back(p->getId());
      overallStats.push_back(*p->getOverallStats());
      roundStats.push_back(*p->getRoundStats());
   }

   mOutgoingPackets.push_back(new GameStatsPacket(ids, overallStats, roundStats));
}

//-----------------------------------------------------------------------------
/*!
   \param direction the bomb direction
   \param player the player who was killed
*/
void Game::rotateDeadPlayerTowardsBomb(Constants::Direction detonationDirection, Player* player, int x, int y)
{
   std::vector<Constants::Direction> dirs;

   // the bomb direction is our first choice
   dirs.push_back(detonationDirection);

   // the directions 90° to that one our 2nd
   switch (detonationDirection)
   {
      case Constants::DirectionLeft:
         dirs.push_back(Constants::DirectionUp);
         dirs.push_back(Constants::DirectionDown);
         break;
      case Constants::DirectionRight:
         dirs.push_back(Constants::DirectionUp);
         dirs.push_back(Constants::DirectionDown);
         break;
      case Constants::DirectionUp:
         dirs.push_back(Constants::DirectionLeft);
         dirs.push_back(Constants::DirectionRight);
         break;
      case Constants::DirectionDown:
         dirs.push_back(Constants::DirectionLeft);
         dirs.push_back(Constants::DirectionRight);
         break;
      default:
         break;
   }

   Constants::Direction testDir = Constants::DirectionUnknown;
   bool hitSomething = false;
   for (Constants::Direction dir : dirs)
   {
      int checkX = x;
      int checkY = y;
      hitSomething = false;
      MapItem* checkItem = nullptr;

      // check if "fall direction" is blocked
      switch (dir)
      {
         case Constants::DirectionLeft:
            checkX--;
            break;
         case Constants::DirectionRight:
            checkX++;
            break;
         case Constants::DirectionUp:
            checkY--;
            break;
         case Constants::DirectionDown:
            checkY++;
            break;
         default:
            break;
      }

      // check if the dead player will hit something
      if (checkX >= 0 && checkX < mMap->getWidth() && checkY >= 0 && checkY < mMap->getHeight())
      {
         checkItem = mMap->getItem(checkX, checkY);

         if (checkItem && checkItem->isBlocking())
         {
            hitSomething = true;
         }
      }

      if (!hitSomething)
      {
         testDir = dir;
         break;
      }
   }

   // it 3 other directions fail => let the player fall towards the bomb
   if (testDir == Constants::DirectionUnknown)
   {
      switch (detonationDirection)
      {
         case Constants::DirectionLeft:
            testDir = Constants::DirectionRight;
            break;
         case Constants::DirectionRight:
            testDir = Constants::DirectionLeft;
            break;
         case Constants::DirectionUp:
            testDir = Constants::DirectionDown;
            break;
         case Constants::DirectionDown:
            testDir = Constants::DirectionUp;
            break;
         default:
            break;
      }
   }

   if (testDir == Constants::DirectionRight)
      player->setKeysPressed(Constants::KeyLeft);
   else if (testDir == Constants::DirectionLeft)
      player->setKeysPressed(Constants::KeyRight);
   else if (testDir == Constants::DirectionUp)
      player->setKeysPressed(Constants::KeyDown);
   else if (testDir == Constants::DirectionDown)
      player->setKeysPressed(Constants::KeyUp);
}

//-----------------------------------------------------------------------------
/*!
   \param bomb bomp map item
*/
void Game::bombExploded(BombMapItem* bomb, bool /*unused*/)
{
   if (getState() != Constants::GameStopped)
   {
      std::vector<Packet*> removeItems;

      // init
      MapItem* mapItem = nullptr;

      // player may drop one more bomb
      Player* player = nullptr;
      {
         auto playerIt = mPlayers.find(bomb->getPlayerId());
         if (playerIt != mPlayers.end())
            player = playerIt->second;
      }

      // player may already be killed :)
      if (player)
         player->setBombsDroppedCount(player->getBombsDroppedCount() - 1);

      int flameCount = bomb->getFlames();
      int x = bomb->getX();
      int y = bomb->getY();

      std::vector<Constants::Direction>* directions;
      directions = &mDirectionCheckCenter;

      bool doneUp = (bomb->getDetonationOrigin() == BombMapItem::Top);
      bool doneDown = (bomb->getDetonationOrigin() == BombMapItem::Bottom);
      bool doneLeft = (bomb->getDetonationOrigin() == BombMapItem::Left);
      bool doneRight = (bomb->getDetonationOrigin() == BombMapItem::Right);

      bool checkGameOver = false;

      // init detonation packet information
      int detonationUp = flameCount;
      int detonationDown = flameCount;
      int detonationLeft = flameCount;
      int detonationRight = flameCount;

      int xDist = 0;
      int yDist = 0;

      // simple stuff: if bomb covered an extra, destroy it
      MapItem* shadowedItem = bomb->getShadowedItem();
      if (shadowedItem)
      {
         removeItems.push_back(new MapItemDestroyedPacket(shadowedItem, bomb->getPlayerId(), Constants::DirectionUnknown, flameCount));

         shadowedItem->setCurrentlyDestroyed(true);
         mDestroyedMapItems.insert(shadowedItem);

         // create appropriate game event
         GameEventPacket* gameEventPacket = new GameEventPacket(GameEventPacket::ExtraDestroyed, 1.0f, x, y);

         mOutgoingPackets.push_back(gameEventPacket);
      }

      // go beginning from the center into each direction
      for (int flameIndex = 0; flameIndex <= flameCount; flameIndex++)
      {
         // center only needs one direction to be checked
         // append the others, after the center element has been processed
         if (flameIndex == 1)
         {
            directions = &mDirectionCheckAll;
         }

         for (int dirIndex = 0; dirIndex < static_cast<int>(directions->size()); dirIndex++)
         {
            Constants::Direction direction = directions->at(dirIndex);

            bool processDirection = true;

            xDist = x;
            yDist = y;

            // calculate next position
            switch (direction)
            {
               case Constants::DirectionUp:
               {
                  if (doneUp)
                     processDirection = false;

                  yDist = y - flameIndex;
                  break;
               }

               case Constants::DirectionDown:
               {
                  if (doneDown)
                     processDirection = false;

                  yDist = y + flameIndex;
                  break;
               }

               case Constants::DirectionLeft:
               {
                  if (doneLeft)
                     processDirection = false;

                  xDist = x - flameIndex;
                  break;
               }

               case Constants::DirectionRight:
               {
                  if (doneRight)
                     processDirection = false;

                  xDist = x + flameIndex;
                  break;
               }

               default:
               {
                  break;
               }
            }

            if (processDirection && xDist >= 0 && yDist >= 0 && xDist < mMap->getWidth() && yDist < mMap->getHeight())
            {
               // check for players that are eventually killed
               for (const auto& [playerId, currentPlayer] : mPlayers)
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

                  //               float playerPosX = currentPlayer->getX();
                  //               float playerPosY = currentPlayer->getY();
                  //
                  //               int pXminusE = std::floor(playerPosX - 0.001);
                  //               int pX       = std::floor(playerPosX);
                  //               int pXplusE  = std::floor(playerPosX + 0.001);
                  //
                  //               int pYminusE = std::floor(playerPosY - 0.001);
                  //               int pY       = std::floor(playerPosY);
                  //               int pYplusE  = std::floor(playerPosY + 0.001);
                  //
                  //               if (
                  //                     xDist == pXminusE && xDist == pX && xDist == pXplusE
                  //                  && yDist == pYminusE && yDist == pY && yDist == pYplusE
                  //              )

                  float pX = currentPlayer->getX();
                  float pY = currentPlayer->getY();

                  /*
                     // recently used, but finally removed when all doubles have
                     // been replaced by floats

                  if (std::floor(pX) < std::floor(pX + 0.001f))
                     pX += 0.001f;
                  if (std::floor(pY) < std::floor(pY + 0.001f))
                     pY += 0.001f;
                  */

                  int currentPlayerX = static_cast<int32_t>(std::floor(pX));
                  int currentPlayerY = static_cast<int32_t>(std::floor(pY));

                  if (currentPlayerX == xDist && currentPlayerY == yDist)
                  {
                     // kill player - once ;)
                     if (!currentPlayer->isKilled())
                     {
                        if (!currentPlayer->isInvincible())
                        {
                           currentPlayer->setKilled(true);

                           // rotate killed player towards the bomb that killed him
                           rotateDeadPlayerTowardsBomb(direction, currentPlayer, xDist, yDist);

                           // if the bomb has been originally detonated by another player, then
                           // we have a different killer. the player who dropped the first bomb
                           // in the detonation chain is considered "the killer".
                           Player* killer = player;

                           if (bomb->getIgniterId() != -1 && bomb->getIgniterId() != player->getId())
                           {
                              killer = nullptr;
                              auto igniterIt = mPlayers.find(bomb->getIgniterId());
                              if (igniterIt != mPlayers.end())
                                 killer = igniterIt->second;
                           }

                           // update stats
                           updateStatsPlayerKilled(killer, currentPlayer);

                           playerKilledSignal(currentPlayer->getId());

                           // create player killed packet
                           mOutgoingPackets.push_back(
                              new PlayerKilledPacket(currentPlayer->getId(), bomb->getPlayerId(), direction, bomb->getFlames())
                           );

                           // show player killed message
                           broadcastMessage(std::format("{} was killed.", currentPlayer->getNick()));

                           // check if game is over
                           checkGameOver = true;
                        }
                     }
                  }
               }

               // ignite bombs that are kicked right into a detonation
               BombKickAnimation::ignite(xDist, yDist);

               // check for mapitems that need to be removed
               mapItem = mMap->getItem(xDist, yDist);

               // if a mapitem is found or the field is immune, stop the
               // detonation
               bool stopDetonation = false;
               if (isFieldImmune(xDist, yDist))
                  stopDetonation = true;

               // if there is a mapitem in the way
               // which is not the bomb that just exploded, then continue
               else if (mapItem && mapItem != bomb)
               {
                  stopDetonation = true;

                  if (mapItem->isDestroyable() && !mapItem->isCurrentlyDestroyed() && mapItem->getType() != MapItem::Bomb)
                  {
                     // map item was destroyed
                     removeItems.push_back(new MapItemDestroyedPacket(mapItem, bomb->getPlayerId(), direction, flameCount));

                     if (mapItem->getType() == MapItem::Extra)
                     {
                        // create appropriate game event
                        GameEventPacket* gameEventPacket =
                           new GameEventPacket(GameEventPacket::ExtraDestroyed, 1.0f, mapItem->getX(), mapItem->getY());

                        mOutgoingPackets.push_back(gameEventPacket);
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
                     mapItem->setCurrentlyDestroyed(true);

                     mDestroyedMapItems.insert(mapItem);
                  }

                  // initiate surrounding bombs
                  if (mapItem->getType() == MapItem::Bomb && !mapItem->isCurrentlyDestroyed())
                  {
                     // neighboured bombs explode from another bomb's explosion
                     BombMapItem* neighbourBomb = dynamic_cast<BombMapItem*>(mapItem);

                     // origin: left
                     if (neighbourBomb->getX() > mapItem->getX())
                        neighbourBomb->setDetonationOrigin(BombMapItem::Left);

                     // origin: right
                     if (neighbourBomb->getX() < mapItem->getX())
                        neighbourBomb->setDetonationOrigin(BombMapItem::Right);

                     // origin: top
                     if (neighbourBomb->getY() > mapItem->getY())
                        neighbourBomb->setDetonationOrigin(BombMapItem::Top);

                     // origin: bottom
                     if (neighbourBomb->getY() < mapItem->getY())
                        neighbourBomb->setDetonationOrigin(BombMapItem::Bottom);

                     // trigger recursive explosion
                     if (neighbourBomb->getInterval() > SERVER_BOMB_NEIGHBOUR_DELAY)
                     {
                        // the id of the player who ignited another bomb
                        // with his own one is inherited here; this is done
                        // in order to compute a proper score for the kills.
                        neighbourBomb->setIgniterId(bomb->getPlayerId());

                        // reduce timer interval in order to make the bomb
                        // explode pretty soon
                        neighbourBomb->setInterval(SERVER_BOMB_NEIGHBOUR_DELAY);
                     }
                  }
               }

               if (stopDetonation)
               {
                  // in any case:
                  // stop explosion into the given direction
                  switch (direction)
                  {
                     case Constants::DirectionUp:
                     {
                        doneUp = true;
                        detonationUp = flameIndex /*- 1*/;
                        break;
                     }

                     case Constants::DirectionDown:
                     {
                        doneDown = true;
                        detonationDown = flameIndex /*- 1*/;
                        break;
                     }

                     case Constants::DirectionLeft:
                     {
                        doneLeft = true;
                        detonationLeft = flameIndex /*- 1*/;
                        break;
                     }

                     case Constants::DirectionRight:
                     {
                        doneRight = true;
                        detonationRight = flameIndex /*- 1*/;
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
      }

      // cut off explosions on the map's borders
      // this is just done for visual purposes
      if (x + detonationRight >= mMap->getWidth())
         detonationRight -= (x + detonationRight - mMap->getWidth() + 1);

      if (x - detonationLeft < 0)
         detonationLeft = x;

      if (y + detonationDown >= mMap->getHeight())
         detonationDown -= (y + detonationDown - mMap->getHeight() + 1);

      if (y - detonationUp < 0)
         detonationUp = y;

      /*
      qDebug(
         "Game::bombExploded(): at: (%d, %d) "
         "up %d, down %d, left %d, right %d",
         x,
         y,
         detonationUp,
         detonationDown,
         detonationLeft,
         detonationRight
      );
      */

      // a destroyed item may currently be some other, still-kicked bomb's shadowed item
      // (the item it's temporarily covering on the grid) - QPointer used to auto-null that
      // reference when the pointee died; now that mShadowedItem is a plain raw pointer, we
      // have to invalidate it explicitly before deleting, same pattern already used for
      // Game::mSpectators.
      for (int gx = 0; gx < mMap->getWidth(); gx++)
      {
         for (int gy = 0; gy < mMap->getHeight(); gy++)
         {
            MapItem* gridItem = mMap->getItem(gx, gy);

            if (gridItem && gridItem->getType() == MapItem::Bomb)
            {
               BombMapItem* otherBomb = dynamic_cast<BombMapItem*>(gridItem);

               if (otherBomb && mDestroyedMapItems.count(otherBomb->getShadowedItem()))
               {
                  otherBomb->setShadowedItem(nullptr);
               }
            }
         }
      }

      // cleanup map when all explosions are finished
      for (MapItem* destroyedItem : mDestroyedMapItems)
      {
         mMap->setItem(destroyedItem->getX(), destroyedItem->getY(), nullptr);

         if (destroyedItem->getType() == MapItem::Stone)
         {
            makeFieldImmune(destroyedItem->getX(), destroyedItem->getY());

            StoneMapItem* stone = dynamic_cast<StoneMapItem*>(destroyedItem);
            std::unique_ptr<ExtraMapItem> extra = stone->releaseExtraMapItem();

            if (extra)
            {
               ExtraMapItem* extraRaw = extra.release();

               mMap->setItem(destroyedItem->getX(), destroyedItem->getY(), extraRaw);

               // send mapitem
               mOutgoingPackets.push_back(new ExtraMapItemCreatedPacket(extraRaw));

               extraRaw->initializeStartTime();
            }
         }
      }

      for (MapItem* destroyedItem : mDestroyedMapItems)
      {
         delete destroyedItem;
      }

      mDestroyedMapItems.clear();

      if (checkGameOver)
      {
         // a player died - so check if there are players left in the game
         updateGameoverCondition();
      }

      // play a bomb sample
      float intensity = detonationUp + detonationDown + detonationLeft + detonationRight;

      intensity *= 0.03125f;

      mOutgoingPackets.push_back(new GameEventPacket(GameEventPacket::BombExploded, intensity));

      // first the detonation packet
      mOutgoingPackets.push_back(new DetonationPacket(x, y, detonationUp, detonationDown, detonationLeft, detonationRight, flameCount));

      // remove bombmapitem
      mOutgoingPackets.push_back(new MapItemRemovedPacket(bomb));

      // remove bomb from the map
      mMap->setItem(bomb->getX(), bomb->getY(), nullptr);

      // this runs synchronously from inside bomb's own explodedSignal dispatch (see the
      // connect() site above) - destroying it here would destroy the object while one of its
      // own methods is still on the call stack. Defer to the next tick instead (same cadence
      // deleteLater() used to provide), same pattern as BombKickAnimation::explodeDelayed().
      Timer::singleShot(0, [bomb]() { delete bomb; });

      // then the "item destroyed" packets
      mOutgoingPackets.insert(mOutgoingPackets.end(), removeItems.begin(), removeItems.end());
   }
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if game is only populated by bots
*/
bool Game::isGamePopulatedByBots() const
{
   bool botsOnly = true;

   for (const auto& [id, currentPlayer] : mPlayers)
   {
      if (!currentPlayer->isBot() && !currentPlayer->isKilled())
      {
         botsOnly = false;
         break;
      }
   }

   return botsOnly;
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::updateGameoverCondition()
{
   int alivePlayerCount = 0;
   Player* potentialWinner = nullptr;

   // check for players that are eventually killed
   for (const auto& [id, currentPlayer] : mPlayers)
   {
      if (!currentPlayer->isKilled())
      {
         potentialWinner = currentPlayer;
         alivePlayerCount++;
      }
   }

   // the game is over
   if (alivePlayerCount <= 1)
   {
      processPlayerWon(potentialWinner);
      finishGame();
   }

   // only bots left
   else if (isGamePopulatedByBots())
   {
      processOnlyBotsLeft();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::resetRoundStats()
{
   for (const auto& [id, player] : mPlayers)
   {
      player->getRoundStats()->reset();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::prepareGame()
{
   // tell clients to clear their maps
   initMapRelatedItems();

   // start countdown
   if (mSkipCountdown)
   {
      startGame();
   }
   else
   {
      // update game state
      setState(Constants::GamePreparing);

      // start preparation timer
      mPreparationTime.restart();
      mPreparationTimer.start();

      mPreparationCounter = SERVER_PREPARATION_TIME + SERVER_PREPARATION_SYNC_TIME;

      // first countdown is called immediately
      updatePrepareGame();
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::updatePrepareGame()
{
   mPreparationCounter--;

   mOutgoingPackets.push_back(new CountdownPacket(mPreparationCounter));

   if (mPreparationCounter == 0)
   {
      qDebug("Game::updatePrepareGame: starting game now");

      // stop preparation timer
      mPreparationTimer.stop();

      // start the game
      startGame();
   }
   else
   {
      qDebug("Game::updatePrepareGame: starting game in %d secs", mPreparationCounter);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::finishGame()
{
   if (getState() != Constants::GameFinishing)
   {
      setState(Constants::GameFinishing);

      mMap->stopBombs();

      // this is the time to display some sort of finish animation

      Timer::singleShot(SERVER_FINISHING_TIME, [this]() { stopGame(); });
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::stopGame()
{
   setState(Constants::GameStopped);
   setStartPositionsInitialized(false);

   bool finished = false;
   mRunning = false;

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
      finished = mGameRound.isFinished();
   }

   if (finished)
   {
      // send and rounds stats if a round is finished
      broadcastGameStats();
      resetRoundStats();

      // reset count and finished flags, we're done.
      mGameRound.reset();
   }

   // send updated game information to everyone (this is mainly done to
   // communicate the updated round count)
   broadcastGameInformation();

   // broadcast "gameover" to all players
   // the "finished" flag is added to let the client know whether to switch
   // back to the lounge or to stay in the win drawable
   mOutgoingPackets.push_back(new StopGameResponsePacket(mGameId, finished));
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::nextRound()
{
   mGameRound.next();

   // restart game until all rounds are finished
   if (!mGameRound.isFinished())
   {
      int delay = SHOW_WINNER_DISPLAY_TIME + SHOW_WINNER_FADE_IN_TIME + SHOW_WINNER_FADE_OUT_TIME + SHOW_WINNER_ADDITIONAL_TIME;

      Timer::singleShot(delay, [this]() { prepareGame(); });
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::startSynchronization()
{
   if (!isSynchronizationActive())
   {
      mSynchronizationTime.restart();
      setSynchronizationActive(true);

      synchronize();
   }
}

//-----------------------------------------------------------------------------
/*!
   \param active synchronization active flag
*/
void Game::setSynchronizationActive(bool active)
{
   mSynchronizationActive = active;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if synchronization is active
*/
bool Game::isSynchronizationActive() const
{
   return mSynchronizationActive;
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::synchronize()
{
   bool synchronized = true;

   // check is all players already sent their "sync" flag
   std::vector<Player*> players = getPlayers();
   std::vector<Player*> criticalPlayers;
   int botCount = 0;

   for (Player* player : players)
   {
      if (player->isBot())
         botCount++;

      if (!player->isLoadingSynchronized())
      {
         //         qDebug(
         //            "Game::synchronize(): waiting for '%s'",
         //            qPrintable(player->getNick())
         //         );

         synchronized = false;
         criticalPlayers.push_back(player);
      }
   }

   if (synchronized)
   {
      setSynchronizationActive(false);
      prepareGame();
   }
   else
   {
      if (mSynchronizationTime.elapsed() < mSyncMaxTime)
      {
         // otherwise wait and retry
         Timer::singleShot(100, [this]() { synchronize(); });
      }
      else
      {
         setSynchronizationActive(false);

         // time has elapsed, kick those who died trying
         for (Player* player : criticalPlayers)
         {
            qDebug("Game::synchronize: time to kick '%s' out of the game", player->getNick().c_str());

            NET_StreamSocket* socket = nullptr;
            for (const auto& [candidateSocket, candidatePlayer] : mPlayerSockets)
            {
               if (candidatePlayer == player)
               {
                  socket = candidateSocket;
                  break;
               }
            }
            forceLeaveGameSignal(socket);

            ErrorPacket* errorPacket = new ErrorPacket(Constants::ErrorSyncTimeout, "sync aborted.;your pc is too slow.");

            sendPacket(socket, errorPacket);
         }

         int playersLeftCount = static_cast<int>(players.size()) - static_cast<int>(criticalPlayers.size());
         int humanPlayersLeftCount = playersLeftCount - botCount;

         // let the others play (if they're more than 2 guys and not only bots)
         if (playersLeftCount > 1 && playersLeftCount > botCount)
         {
            prepareGame();
         }
         else
         {
            if (humanPlayersLeftCount > 0)
            {
               // there's no use running a game with one player in it
               // therefore we stop it.
               stopGame();
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   send single packet
*/
void Game::sendPacket(NET_StreamSocket* socket, Packet* packet)
{
   // init bytearray
   packet->serialize();

   /*
   qDebug(
      "Game::sendPacket: sending packet to player %p (%d bytes)",
      socket,
      packet->size()
   );
   */

   // send packet
   if (socket)
      NET_WriteToStreamSocket(socket, packet->constData(), static_cast<int>(packet->size()));

   // clean up
   delete packet;
}

//-----------------------------------------------------------------------------
/*!
   send outgoing packets
*/
void Game::sendBroadcastPackets()
{
   if (!mOutgoingPackets.empty())
   {
      bool synced = false;
      Packet* packet = nullptr;
      Packet::TYPE pType = Packet::INVALID;

      // serialize each packet exactly once - Packet::serialize() appends to the packet's own
      // byte buffer rather than resetting it, so calling it once per connected socket (as the
      // loop below used to) kept re-appending the same payload to itself, corrupting every
      // socket's stream past the first with duplicated packets.
      for (size_t i = 0; i < mOutgoingPackets.size(); ++i)
      {
         mOutgoingPackets.at(i)->serialize();
      }

      for (const auto& [socket, socketPlayer] : mPlayerSockets)
      {
         synced = socketPlayer->isLoadingSynchronized();

         // write outgoing packets to socket
         for (size_t i = 0; i < mOutgoingPackets.size(); ++i)
         {
            if (socket)
            {
               packet = mOutgoingPackets.at(i);
               pType = packet->getType();

               if (synced || pType == Packet::JOINGAMERESPONSE || pType == Packet::MESSAGE)
               {
                  NET_WriteToStreamSocket(socket, packet->constData(), static_cast<int>(packet->size()));
               }
            }
         }
      }

      // clean up
      while (!mOutgoingPackets.empty())
      {
         delete mOutgoingPackets.front();
         mOutgoingPackets.pop_front();
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param tcpSocket tcp socket
   \param packet packet to process
*/
void Game::processPacket(NET_StreamSocket* tcpSocket, Packet* packet)
{
   switch (packet->getType())
   {
      case Packet::MESSAGE:
      {
         MessagePacket* senderPacket = dynamic_cast<MessagePacket*>(packet);

         // during the game we don't care if any of the players is currently
         // typing something. this is just relevant for the lounge
         bool broadcastAllowed = (mRunning && senderPacket->isTypingFinished()) || !mRunning;

         if (broadcastAllowed)
         {
            int senderId = mPlayerSockets[tcpSocket]->getId();

            for (const auto& [currentSocket, currentPlayer] : mPlayerSockets)
            {
               // send message to all players unless it's private
               if (senderPacket->getReceiverId() == -1 || senderPacket->getReceiverId() == currentPlayer->getId())
               {
                  MessagePacket* messagePacket = new MessagePacket(
                     senderId,
                     std::format("{}: {}", mPlayerSockets[tcpSocket]->getNick(), StringUtils::trim(senderPacket->getMessage())),
                     senderPacket->isTypingFinished(),
                     senderPacket->getReceiverId()
                  );

                  sendPacket(currentSocket, messagePacket);
               }
            }
         }

         break;
      }

      case Packet::KEY:
      {
         KeyPacket* keyPacket = dynamic_cast<KeyPacket*>(packet);

         Player* player = mPlayerSockets[tcpSocket];

         if (player && !player->isKilled() && getState() == Constants::GameActive)
         {
            // fix up the player's inputs to not get confused in
            // any way (the player may to go to the left and to
            // the right at the same time for example)
            int8_t keys = keyPacket->getKeys();
            int8_t previousKeys = static_cast<int8_t>(player->getKeysPressed());

            if ((previousKeys & Constants::KeyRight) && (keys & Constants::KeyLeft))
            {
               keys &= ~(Constants::KeyRight);
            }

            if ((previousKeys & Constants::KeyLeft) && (keys & Constants::KeyRight))
            {
               keys &= ~(Constants::KeyLeft);
            }

            if ((previousKeys & Constants::KeyUp) && (keys & Constants::KeyDown))
            {
               keys &= ~(Constants::KeyUp);
            }

            if ((previousKeys & Constants::KeyDown) && (keys & Constants::KeyUp))
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

            //            qDebug(
            //               "Game::data: key packet processed: pid: %d, keys: %d, prev: %d",
            //               keyPacket->getPlayerId(),
            //               keys,
            //               previousKeys
            //            );
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
            Player* player = mPlayerSockets[tcpSocket];

            if (player)
            {
               if (getCreator() == player)
               {
                  finishGame();

                  // show player killed message
                  broadcastMessage(std::format("{} aborted the game.", player->getNick()));
               }
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

//----------------------------------------------------------------------------
/*!
   \return game id
*/
int Game::getId() const
{
   return mGameId;
}

//----------------------------------------------------------------------------
/*!
   \return game name
*/
const std::string& Game::getName() const
{
   return mCreateGameData.mName;
}

//----------------------------------------------------------------------------
/*!
   \return level name
*/
const std::string& Game::getLevelName() const
{
   return mCreateGameData.mLevel;
}

//----------------------------------------------------------------------------
/*!
   \return player count
*/
int Game::getPlayerCount() const
{
   return static_cast<int>(mPlayers.size());
}

//----------------------------------------------------------------------------
/*!
   \return maximum player count
*/
int Game::getMaximumPlayerCount() const
{
   return mMap->getMaxPlayers();
}

//----------------------------------------------------------------------------
/*!
   \param data create game data
*/
void Game::setCreateGameData(const CreateGameData& data)
{
   mCreateGameData = data;
}

//----------------------------------------------------------------------------
/*!
   \param name new game name
*/
void Game::setName(const std::string& name)
{
   mCreateGameData.mName = name;
}

//----------------------------------------------------------------------------
/*!
   \param player player who joined the game
*/
bool Game::joinGame(Player* player, NET_StreamSocket* playerSocket)
{
   bool joiningAllowed = false;

   if (getPlayerCount() < getMaximumPlayerCount())
   {
      // assign player color
      player->setColor(getColorForNextPlayer());

      // reset player stats on join game event
      player->resetStats();
      player->setKilled(true);

      // send join information to all other players
      for (const auto& [id, existingPlayer] : mPlayers)
      {
         JoinGameResponsePacket* existingPlayerPacket =
            new JoinGameResponsePacket(true, mGameId, existingPlayer->getId(), existingPlayer->getNick(), existingPlayer->getColor());

         sendPacket(playerSocket, existingPlayerPacket);
      }

      // insert the new player and send him his granted packet
      mPlayers[player->getId()] = player;

      mPlayerSockets[playerSocket] = player;

      mOutgoingPackets.push_back(new JoinGameResponsePacket(true, mGameId, player->getId(), player->getNick(), player->getColor()));

      joiningAllowed = true;

      /*
         that is obsolete code

         // init and broadcast the new player's start position
         // send the player's start position to everybody
         Point startPosition = Point(mPlayers.size() - 1, 0);

         player->setX(startPosition.x() + 0.5);
         player->setY(startPosition.y() + 0.5);

         PositionPacket* positionPacket = new PositionPacket(
            player->getId(),
            Constants::KeyDown,
            player->getX(),
            player->getY(),
            M_PI * 1.5f
         );

         mOutgoingPackets.push_back(positionPacket);
      */
   }
   else
   {
      // game is full
      sendPacket(playerSocket, new JoinGameResponsePacket(false, mGameId, player->getId(), player->getNick(), player->getColor()));
   }

   return joiningAllowed;
}

//-----------------------------------------------------------------------------
/*!
   \param player ptr to player
   \param game ptr to game
   \param tcpSocket player's tcp socket
*/
void Game::processSpectator(NET_StreamSocket* tcpSocket)
{
   if (getState() == Constants::GameActive || getState() == Constants::GamePreparing)
   {
      int timeLeft = getTimeLeft();

      /*
      if (timeLeft > SERVER_MAX_REMAINING_TIME)
      {
         // update player newly joined player (time left)
         sendPacket(
            tcpSocket,
            new TimePacket(getTimeLeft())
         );

         // update player newly joined player (game info)
         QList<GameInformation> games;
         games << getGameInformation();

         sendPacket(
            tcpSocket,
            new ListGamesResponsePacket(
               games,
               true
            )
         );

         // show him the current map
         std::vector<MapItemCreatedPacket*> createPackets = mMap->getMapItemCreatedPackets();
         for (int i = 0; i < static_cast<int>(createPackets.size()); i++)
         {
            sendPacket(
               tcpSocket,
               createPackets[i]
            );
         }

         // make client show the game
         sendPacket(
            tcpSocket,
            new CountdownPacket(
                 SERVER_PREPARATION_TIME
               + SERVER_PREPARATION_SYNC_TIME
               - 1
            )
         );

         // tell him the game has started
         sendPacket(
            tcpSocket,
            new StartGameResponsePacket(getId(), true)
         );
      }
      */

      if (timeLeft > SERVER_MAX_REMAINING_TIME)
      {
         mSpectators.push_back(tcpSocket);

         Timer::singleShot(SERVER_SPECTATOR_DELAY, [this]() { processSpectatorMessage(); });
      }
   }
}

//----------------------------------------------------------------------------
/*!
 */
void Game::processSpectatorMessage()
{
   if (!mSpectators.empty())
   {
      NET_StreamSocket* tcpSocket = mSpectators.front();
      mSpectators.pop_front();

      if (tcpSocket)
      {
         int timeLeft = getTimeLeft();

         // tell player next round will start in n seconds
         MessagePacket* message1 = new MessagePacket(-1, "Please wait, the game is currently active.", true);

         MessagePacket* message2 = new MessagePacket(
            -1,
            (timeLeft == 1) ? std::string("The next game will start in about 1 second.")
                            : std::format("The next game will start in about {} seconds.", timeLeft),
            true
         );

         sendPacket(tcpSocket, message1);
         sendPacket(tcpSocket, message2);
      }
   }
}

//----------------------------------------------------------------------------
/*!
   \return position skip count
*/
int Game::getPositionSkipCount() const
{
   return mPositionSkipCount;
}

//----------------------------------------------------------------------------
/*!
   \return bot count
*/
int Game::getBotCount() const
{
   int bots = 0;
   for (const auto& [id, player] : mPlayers)
   {
      if (player->isBot())
         bots++;
   }
   return bots;
}

//----------------------------------------------------------------------------
/*!
   \return ptr to game round instance
*/
GameRound* Game::getGameRound()
{
   return &mGameRound;
}

//----------------------------------------------------------------------------
/*!
 */
void Game::increasePlayersLeftTheGameCount()
{
   mPlayerLeftTheGameCount++;
}

//----------------------------------------------------------------------------
/*!
 */
void Game::resetPlayersLeftTheGameCount()
{
   mPlayerLeftTheGameCount = 0;
}

//----------------------------------------------------------------------------
/*!
   \return players left game counter
*/
int Game::getPlayersLeftTheGameCount() const
{
   return mPlayerLeftTheGameCount;
}

//----------------------------------------------------------------------------
/*!
   \return \c true if start position is initialized
*/
bool Game::isStartPositionInitialized() const
{
   return mStartPositionsInitialized;
}

//----------------------------------------------------------------------------
/*!
   \param value start position initialized flag
*/
void Game::setStartPositionsInitialized(bool value)
{
   mStartPositionsInitialized = value;
}

//----------------------------------------------------------------------------
/*!
   \param player player who left the game
*/
void Game::removePlayer(Player* player, NET_StreamSocket* playerSocket)
{
   increasePlayersLeftTheGameCount();

   mPlayerSockets.erase(playerSocket);
   mPlayers.erase(player->getId());

   // drop the socket from the spectator queue too, so processSpectatorMessage()'s
   // delayed timer never fires against a socket that's since been destroyed
   mSpectators.erase(std::remove(mSpectators.begin(), mSpectators.end(), playerSocket), mSpectators.end());

   // reset player stats on leave game event
   player->resetStats();

   // also the player needs to synchronize loading
   // the next time he's joining a game
   player->setLoadingSynchronized(false);

   playerLeavesSignal(player->getId());
}

//----------------------------------------------------------------------------
/*!
   \param packet packet to send
*/
void Game::addOutgoingPacket(Packet* packet)
{
   mOutgoingPackets.push_back(packet);
}

//----------------------------------------------------------------------------
/*!
   \return time left
*/
int Game::getTimeLeft()
{
   return static_cast<int32_t>(mCreateGameData.mDuration - (mGameTime.elapsed() * 0.001f));
}

//----------------------------------------------------------------------------
/*!
 */
void Game::processGameTime()
{
   if (getState() == Constants::GameActive)
   {
      int timeLeft = getTimeLeft();

      if (timeLeft <= 0)
      {
         finishGame();
      }

      mOutgoingPackets.push_back(new TimePacket(timeLeft));
   }
}

//----------------------------------------------------------------------------
/*!
 */
void Game::setCreator(Player* creator)
{
   mCreator = creator;
}

//----------------------------------------------------------------------------
/*!
   \return ptr to the game's creator
*/
Player* Game::getCreator() const
{
   return mCreator;
}

//----------------------------------------------------------------------------
/*!
   \return list of players
*/
std::vector<Player*> Game::getPlayers() const
{
   std::vector<Player*> players;
   players.reserve(mPlayers.size());

   for (const auto& [id, player] : mPlayers)
      players.push_back(player);

   return players;
}

//----------------------------------------------------------------------------
/*!
   \param state game state
*/
void Game::setState(Constants::GameState state)
{
   mState = state;
   stateChangedSignal(state);
}

//----------------------------------------------------------------------------
/*!
   \return state
*/
Constants::GameState Game::getState() const
{
   return mState;
}

//----------------------------------------------------------------------------
/*!
 */
void Game::increaseGamesPlayed()
{
   mGamesPlayed++;
}

//----------------------------------------------------------------------------
/*!
   \return number of rounds played
*/
int Game::getGamesPlayed() const
{
   return mGamesPlayed;
}

//----------------------------------------------------------------------------
/*!
   \return color for next player
*/
Constants::Color Game::getColorForNextPlayer() const
{
   Constants::Color color = Constants::ColorWhite;

   std::unordered_set<Constants::Color> colors;
   for (int i = 1; i <= 10; i++)
      colors.insert(static_cast<Constants::Color>(i));

   for (const auto& [id, p] : mPlayers)
      colors.erase(p->getColor());

   std::vector<Constants::Color> colorList(colors.begin(), colors.end());
   std::sort(colorList.begin(), colorList.end());

   if (!colorList.empty())
      color = colorList.front();

   return color;
}

//----------------------------------------------------------------------------
/*!
   \return ptr to player socket map
*/
std::map<NET_StreamSocket*, Player*>* Game::getPlayerSockets()
{
   return &mPlayerSockets;
}

//----------------------------------------------------------------------------
/*!
   \return ptr to game map
*/
Map* Game::getMap() const
{
   return mMap;
}

//----------------------------------------------------------------------------
/*!
   \return map dimensions
*/
Constants::Dimension Game::getMapDimension() const
{
   return mCreateGameData.mDimension;
}

//----------------------------------------------------------------------------
/*!
   \param message message to broadcast
*/
void Game::broadcastMessage(const std::string& message)
{
   mOutgoingPackets.push_back(new MessagePacket(-1, message, true));
}

//----------------------------------------------------------------------------
/*!
   \return list of extras
*/
int Game::getExtras() const
{
   int extras = 0;

   if (mCreateGameData.mExtraBombEnabled)
      extras |= Constants::ExtraBomb;

   if (mCreateGameData.mExtraFlameEnabled)
      extras |= Constants::ExtraFlame;

   if (mCreateGameData.mExtraSpeedupEnabled)
      extras |= Constants::ExtraSpeedup;

   if (mCreateGameData.mExtraKickEnabled)
      extras |= Constants::ExtraKick;

   if (mCreateGameData.mExtraSkullsEnabled)
      extras |= Constants::ExtraSkull;

   return extras;
}

//----------------------------------------------------------------------------
/*!
   \return game duration
*/
int Game::getDuration() const
{
   return mCreateGameData.mDuration;
}

//-----------------------------------------------------------------------------
/*!
   \param direction move direction
   \param speed move speed
*/
void Game::bombKickedAnimation(BombMapItem* item, Constants::Direction direction, float speed)
{
   if (item)
   {
      mOutgoingPackets.push_back(new MapItemMovePacket(item->getUniqueId(), speed, direction, item->getX(), item->getY()));
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::initializeExtraSpawn()
{
   mExtraSpawn = std::make_unique<ExtraSpawn>();
   mExtraSpawn->setMap(getMap());

   mExtraSpawn->spawnSignal.connect([this]() { spawn(); });
}

//-----------------------------------------------------------------------------
/*!
 */
void Game::spawn()
{
   // only spawn extras in active state
   if (getState() == Constants::GameActive)
   {
      int x = 0;
      int y = 0;
      int px = 0;
      int py = 0;
      bool done = false;
      MapItem* item = nullptr;
      Map* map = getMap();
      int width = map->getWidth();
      int height = map->getHeight();

      while (!done)
      {
         x = Random::bounded(width);
         y = Random::bounded(height);

         item = map->getItem(x, y);

         // there must be no item at x,y
         if (!item)
         {
            // maybe we're done
            done = true;

            // there must be no player at x,y
            for (const auto& [id, p] : mPlayers)
            {
               if (!p->isKilled())
               {
                  px = static_cast<int32_t>(std::floor(p->getX()));
                  py = static_cast<int32_t>(std::floor(p->getY()));

                  if (px == x && py == y)
                  {
                     // nope, neeeeeext!
                     done = false;
                     break;
                  }
               }
            }
         }
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if extra spawning is enabled
*/
bool Game::isSpawnExtrasEnabled() const
{
   return mExtraSpawnEnabled;
}
