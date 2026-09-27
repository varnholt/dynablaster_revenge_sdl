#ifndef BOMBCHAINREACTION_H
#define BOMBCHAINREACTION_H

// ai
class BotMap;

// shared
#include "botbombmapitem.h"
#include "point.h"

#include <unordered_set>
#include <vector>


class BombChainReaction
{
   public:

      typedef std::vector<std::vector<BotBombMapItem*>> ChainList;

      //! constructor
      BombChainReaction();

      //! setter for bot map
      void setBotMap(BotMap* map);

      //! compute chained bombs
      void compute();

      //! do unit test
      void unitTest1();

      //! initialize list of directions
      void initDirections();

      //! getter for detonation chain
      const ChainList& getDetonationChain();


   protected:

      //! recursion
      void iterate(BotBombMapItem *item, std::vector<BotBombMapItem *> &items);

      //! visited items
      std::unordered_set<BotBombMapItem*> mVisited;

      //! bot map
      BotMap* mBotMap;

      //! list of detonation chains
      ChainList mChain;

      //! direction vectors
      std::vector<Point> mDirections;
};

#endif // BOMBCHAINREACTION_H
