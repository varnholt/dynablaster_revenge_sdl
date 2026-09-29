#ifndef BOTFACTORY_H
#define BOTFACTORY_H

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
   std::string _hostname;

   //! game to join
   int _game_id;

   //! list of bot clients
   std::vector<BotClient*> _clients;

   //! list of bots
   std::vector<Bot*> _bots;

   //! list of given names
   std::vector<std::string> _given_names;
};

#endif // BOTFACTORY_H
