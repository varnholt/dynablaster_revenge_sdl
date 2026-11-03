#ifndef BOTFACTORY_H
#define BOTFACTORY_H

#include "bottype.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// forward declarations
class Bot;
class BotClient;
class ByteStream;
struct LearnedBotWeights;

class BotFactory
{
public:
   BotFactory();

   virtual ~BotFactory();

   //! adds bots of the given type, they connect and join the game right away; learned bots of
   //! level n (1 to 5) use data/server/learnedbot_level<n>.txt
   void add(int count = 1, BotType type = BotType::Proto, int level = 3);

   void setHostname(const std::string& hostname);

   const std::string& getHostname() const;

   void setGameId(int game_id);

   int getGameId() const;

   void removeAll();

   void removeBot(BotClient& client);

   //! bots connect through streams made by this instead of the network, e.g. in a headless simulation
   void setLocalConnector(std::function<std::unique_ptr<ByteStream>()> connector);

   //! weights for learned bots added from now on, whatever their level (the sim's --weights)
   void setLearnedWeights(std::shared_ptr<const LearnedBotWeights> weights);

protected:
   //! create a bot and a client instance
   void createBotClientPair(BotType type, int level);

   //! the weights of a level, loaded once
   std::shared_ptr<const LearnedBotWeights> levelWeights(int level);

   //! host to connect to
   std::string _hostname = "127.0.0.1";

   //! game to join
   int _game_id = -1;

   //! list of bot clients (each client owns its bot)
   std::vector<std::unique_ptr<BotClient>> _clients;

   //! removed bot clients, destroyed on the next tick
   std::vector<std::unique_ptr<BotClient>> _removed_clients;

   //! expires with this factory, deferred cleanups check it
   std::shared_ptr<bool> _lifetime = std::make_shared<bool>(true);

   //! makes local streams to the server, empty to connect over the network
   std::function<std::unique_ptr<ByteStream>()> _local_connector;

   //! weights given by setLearnedWeights(), they win over the level files
   std::shared_ptr<const LearnedBotWeights> _learned_weights;

   //! weights per level, loaded on first use
   std::map<int, std::shared_ptr<const LearnedBotWeights>> _level_weights;

   //! list of given names
   std::vector<std::string> _given_names;
};

#endif  // BOTFACTORY_H
