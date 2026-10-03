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

BotFactory::BotFactory() = default;

// bots are deleted in client destructor
BotFactory::~BotFactory() = default;

void BotFactory::createBotClientPair()
{
   // todo read data from command line
   // - bot type
   // - host
   // - game id or command to create game
   // - start game flag or command to wait
   auto client_owner = std::make_unique<BotClient>();
   BotClient& client = *client_owner;
   auto bot_owner = std::make_unique<ProtoBot>(client.getPlayerInfoMap());
   ProtoBot& bot = *bot_owner;

   std::vector<std::string> nicks;

   std::ifstream nicks_file("data/server/botnicks.txt");

   if (nicks_file.is_open())
   {
      std::string line;
      while (std::getline(nicks_file, line))
      {
         if (!StringUtils::trim(line).empty())
         {
            nicks.push_back(line);
         }
      }
   }
   else
   {
      nicks = {"r2d2",     "c3po",    "bender",   "no5",     "t-1000",   "t-800",  "cyclon",   "ramrod",  "h8",     "gort",
               "astroboy", "clank",   "rosie",    "hal9000", "sentinel", "vision", "cyberman", "alpha",   "wall-e", "asimo",
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
         duplicate = std::ranges::find(_given_names, nick) != _given_names.end();
      }

      _given_names.push_back(nick);
   }
   else
   {
      // if the nick list size is smaller than the possible bot maximum,
      // just choose any of the names
      nick = nicks[Random::bounded(static_cast<int>(nicks.size()) - 1)];
   }

   client.setBot(std::move(bot_owner));
   client.setGameId(getGameId());
   client.setHost(getHostname());
   client.setNick(nick);
   client.setAutoJoin(true);
   client.setAutoStart(false);
   client.initialize();
   client.connectToServer();

   bot.startTicking();

   //   // link client to bot and vice versa - ProtoBotInsults is never instantiated (_insults
   //   // stays null), so this never had anything to connect to.
   //   bot->getInsults()->sendMessageSignal.connect([client](const std::string& msg) { client->sendMessage(msg); });

   client.updatePlayerIdSignal.connect([&bot](int id) { bot.updatePlayerId(id); });

   client.updatePlayerPositionSignal.connect([&bot](int id, float x, float y, float angle) { bot.updatePlayerPosition(id, x, y, angle); });

   client.extraShakeSignal.connect([&bot](int id) { bot.extraShake(id); });

   client.gameStartedSignal.connect([&bot]() { bot.wakeUp(); });

   // Bot's own signals only ever connect to a BotClient, never BombermanClient.
   bot.walkSignal.connect([&client](int8_t keys_pressed) { client.walk(keys_pressed); });
   bot.bombSignal.connect([&client]() { client.bomb(); });

   client.markHazardousTemporarySignal.connect([&bot](int x, int y, int ms, int field_count)
                                               { bot.markHazardousTemporary(x, y, ms, field_count); });

   client.bombKickedSignal.connect([&bot](int start_x, int start_y, Constants::Direction direction, int flames)
                                   { bot.bombKicked(start_x, start_y, direction, flames); });

   bot.syncSignal.connect([&client]() { client.deleteObsoleteMapItems(); });

   // be notified if bot is to be removed
   client.removeSignal.connect([this, &client]() { removeBot(client); });

   _clients.push_back(std::move(client_owner));
}

/*!
   \param count number of bots to add
*/
void BotFactory::add(int count)
{
   _given_names.clear();

   for (int i = 0; i < count; i++)
   {
      createBotClientPair();
   }

   _given_names.clear();
}

void BotFactory::setHostname(const std::string& hostname)
{
   _hostname = hostname;
}

const std::string& BotFactory::getHostname() const
{
   return _hostname;
}

void BotFactory::setGameId(int game_id)
{
   _game_id = game_id;
}

int BotFactory::getGameId() const
{
   return _game_id;
}

void BotFactory::removeAll()
{
   _clients.clear();
}

void BotFactory::removeBot(BotClient& client)
{
   auto client_iterator = std::ranges::find_if(_clients, [&client](const auto& owned) { return owned.get() == &client; });

   if (client_iterator != _clients.end())
   {
      // deferred - this fires from inside client's own removeSignal dispatch
      _removed_clients.push_back(std::move(*client_iterator));
      _clients.erase(client_iterator);

      Timer::singleShot(
         0,
         [this, lifetime = std::weak_ptr<bool>(_lifetime)]()
         {
            if (!lifetime.expired())
            {
               _removed_clients.clear();
            }
         }
      );
   }
}
