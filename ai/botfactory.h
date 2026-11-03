#ifndef BOTFACTORY_H
#define BOTFACTORY_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

// forward declarations
class Bot;
class BotClient;
class ByteStream;

class BotFactory
{
public:
   BotFactory();

   virtual ~BotFactory();

   void add(int count = 1);

   void setHostname(const std::string& hostname);

   const std::string& getHostname() const;

   void setGameId(int game_id);

   int getGameId() const;

   void removeAll();

   void removeBot(BotClient& client);

   //! bots connect through streams made by this instead of the network, e.g. in a headless simulation
   void setLocalConnector(std::function<std::unique_ptr<ByteStream>()> connector);

protected:
   //! create a bot and a client instance
   void createBotClientPair();

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

   //! list of given names
   std::vector<std::string> _given_names;
};

#endif  // BOTFACTORY_H
