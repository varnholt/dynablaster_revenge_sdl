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
      std::unordered_set<BotBombMapItem*> _visited;

      //! bot map
      BotMap* _bot_map;

      //! list of detonation chains
      ChainList _chain;

      //! direction vectors
      std::vector<Point> _directions;
};

#endif // BOMBCHAINREACTION_H
