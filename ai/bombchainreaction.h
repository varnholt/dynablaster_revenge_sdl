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
   using ChainList = std::vector<std::vector<BotBombMapItem*>>;

   //! setter for bot map
   void setBotMap(BotMap* map);

   //! compute chained bombs
   void compute();

   //! initialize list of directions
   void initDirections();

   //! getter for detonation chain
   const ChainList& getDetonationChain() const;

protected:
   //! recursion
   void iterate(BotBombMapItem* item, std::vector<BotBombMapItem*>& items);

   //! visited items
   std::unordered_set<BotBombMapItem*> _visited;

   //! bot map
   BotMap* _bot_map = nullptr;

   //! list of detonation chains
   ChainList _chain;

   //! direction vectors
   std::vector<Point> _directions;
};

#endif  // BOMBCHAINREACTION_H
