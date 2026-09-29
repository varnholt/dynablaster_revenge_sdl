#ifndef BOTFACTORY_H
#define BOTFACTORY_H

#include <memory>
#include <string>
#include <vector>

// forward declarations
class Bot;
class BotClient;

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

   void removeBot(BotClient* client);

protected:
   //! create a bot and a client instance
   void createBotClientPair();

   //! host to connect to
   std::string _hostname = "127.0.0.1";

   //! game to join
   int _game_id = -1;

   //! list of bot clients (each client owns its bot)
   std::vector<std::unique_ptr<BotClient>> _clients;

   //! list of bots, non-owning
   std::vector<Bot*> _bots;

   //! list of given names
   std::vector<std::string> _given_names;
};

#endif  // BOTFACTORY_H
