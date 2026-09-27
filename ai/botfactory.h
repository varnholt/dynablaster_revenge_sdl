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

   void setGameId(int gameId);

   int getGameId() const;

   void removeAll();

   void removeBot(BotClient* client);


protected:

   //! create a bot and a client instance
   void createBotClientPair();

   //! host to connect to
   std::string mHostname;

   //! game to join
   int mGameId;

   //! list of bot clients
   std::vector<BotClient*> mClients;

   //! list of bots
   std::vector<Bot*> mBots;

   //! list of given names
   std::vector<std::string> mGivenNames;
};

#endif // BOTFACTORY_H
