#include "botfactory.h"

#include "random.h"
#include "timer.h"

// bot
#include "botclient.h"
#include "botmap.h"
#include "protobot.h"
#include "protobotinsults.h"
#include "stringutils.h"

#include <algorithm>
#include <fstream>

//-----------------------------------------------------------------------------
/*!
 */
BotFactory::BotFactory() : mGameId(-1)
{
   setHostname("127.0.0.1");
}

//-----------------------------------------------------------------------------
/*!
 */
BotFactory::~BotFactory()
{
   // bots are deleted in client destructor
   for (BotClient* client : mClients)
   {
      delete client;
   }

   mClients.clear();
}

//-----------------------------------------------------------------------------
/*!
 */
void BotFactory::createBotClientPair()
{
   // todo read data from command line
   // - bot type
   // - host
   // - game id or command to create game
   // - start game flag or command to wait
   BotClient* client = new BotClient();
   ProtoBot* bot = new ProtoBot();

   bot->setPlayerInfoMap(client->getPlayerInfoMap());

   std::vector<std::string> nicks;

   std::ifstream nicksFile("data/server/botnicks.txt");

   if (nicksFile.is_open())
   {
      std::string line;
      while (std::getline(nicksFile, line))
      {
         if (!StringUtils::trim(line).empty())
            nicks.push_back(line);
      }
   }
   else
   {
      nicks = {"r2d2",     "c3po",   "bender",   "no5",      "t-1000", "t-800",  "cyclon",  "ramrod",  "h8",     "gort",
               "astroboy", "clank",  "rosie",    "hal9000",  "sentinel", "vision", "cyberman", "alpha",   "wall-e", "asimo",
               "cylon",    "voltron", "wheatley", "megaman", "brainiac", "eve",    "pneuman",  "optimus", "robby",  "awesome-o"};
   }

   std::string nick;

   if (nicks.size() >= 9)
   {
      // if we have sufficient nicks in that list we'll keep choosing new
      // nicks until we'll have found a non-duplicate nick
      bool duplicate = true;

      while (duplicate)
      {
         nick = nicks[Random::bounded(static_cast<int>(nicks.size()) - 1)];
         duplicate = std::find(mGivenNames.begin(), mGivenNames.end(), nick) != mGivenNames.end();
      }

      mGivenNames.push_back(nick);
   }
   else
   {
      // if the nick list size is smaller than the possible bot maximum,
      // just choose any of the names
      nick = nicks[Random::bounded(static_cast<int>(nicks.size()) - 1)];
   }

   client->setBot(bot);
   client->setGameId(getGameId());
   client->setHost(getHostname());
   client->setNick(nick);
   client->setAutoJoin(true);
   client->setAutoStart(false);
   client->initialize();
   client->connectToServer();

   bot->startTicking();

   //   // link client to bot and vice versa - ProtoBotInsults is never instantiated (mInsults
   //   // stays null), so this never had anything to connect to.
   //   bot->getInsults()->sendMessageSignal.connect([client](const std::string& msg) { client->sendMessage(msg); });

   client->updatePlayerIdSignal.connect([bot](int id) { bot->updatePlayerId(id); });

   client->updatePlayerPositionSignal.connect([bot](int id, float x, float y, float ang) { bot->updatePlayerPosition(id, x, y, ang); });

   client->extraShakeSignal.connect([bot](int id) { bot->extraShake(id); });

   client->gameStartedSignal.connect([bot]() { bot->wakeUp(); });

   // Bot's own signals only ever connect to a BotClient, never BombermanClient.
   bot->walkSignal.connect([client](int8_t keysPressed) { client->walk(keysPressed); });
   bot->bombSignal.connect([client]() { client->bomb(); });

   client->markHazardousTemporarySignal.connect(
      [bot](int x, int y, int ms, int fieldCount) { bot->markHazardousTemporary(x, y, ms, fieldCount); }
   );

   client->bombKickedSignal.connect(
      [bot](int startX, int startY, Constants::Direction dir, int flames) { bot->bombKicked(startX, startY, dir, flames); }
   );

   bot->syncSignal.connect([client]() { client->deleteObsoleteMapItems(); });

   // be notified if bot is to be removed
   client->removeSignal.connect([this, client]() { removeBot(client); });

   mClients.push_back(client);
   mBots.push_back(bot);
}

//-----------------------------------------------------------------------------
/*!
   \param count number of bots to add
*/
void BotFactory::add(int count)
{
   mGivenNames.clear();

   for (int i = 0; i < count; i++)
      createBotClientPair();

   mGivenNames.clear();
}

//-----------------------------------------------------------------------------
/*!
 */
void BotFactory::setHostname(const std::string& hostname)
{
   mHostname = hostname;
}

//-----------------------------------------------------------------------------
/*!
 */
const std::string& BotFactory::getHostname() const
{
   return mHostname;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotFactory::setGameId(int gameId)
{
   mGameId = gameId;
}

//-----------------------------------------------------------------------------
/*!
 */
int BotFactory::getGameId() const
{
   return mGameId;
}

//-----------------------------------------------------------------------------
/*!
 */
void BotFactory::removeAll()
{
   for (BotClient* client : mClients)
   {
      delete client;
   }

   mClients.clear();
   mBots.clear();
}

//-----------------------------------------------------------------------------
/*!
 */
void BotFactory::removeBot(BotClient* client)
{
   if (client)
   {
      auto it = std::find(mClients.begin(), mClients.end(), client);

      if (it != mClients.end())
      {
         mClients.erase(it);
      }

      // deferred - this fires from inside client's own removeSignal dispatch
      Timer::singleShot(0, [client]() { delete client; });
   }
}
