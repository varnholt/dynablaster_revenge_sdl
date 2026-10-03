#ifndef BOMBCHAINREACTION_H
#define BOMBCHAINREACTION_H

// ai
class BotMap;

// shared
#include "botbombmapitem.h"
#include "point.h"

#include <cstdint>
#include <functional>
#include <unordered_set>
#include <vector>

class BombChainReaction
{
public:
   using ChainList = std::vector<std::vector<std::reference_wrapper<const BotBombMapItem>>>;

   //! compute chained bombs of the given map, valid while the map is unchanged
   void compute(const BotMap& map);

   //! initialize list of directions
   void initDirections();

   //! getter for detonation chain
   const ChainList& getDetonationChain() const;

protected:
   //! recursion
   void iterate(const BotMap& map, const BotBombMapItem& item, std::vector<std::reference_wrapper<const BotBombMapItem>>& items);

   //! unique ids of the visited items
   std::unordered_set<int32_t> _visited;

   //! list of detonation chains
   ChainList _chain;

   //! direction vectors
   std::vector<Point> _directions;
};

#endif  // BOMBCHAINREACTION_H
