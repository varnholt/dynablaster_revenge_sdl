#include "bombchainreaction.h"

// ai
#include "botmap.h"

/*!
   \param map map to set
*/
void BombChainReaction::setBotMap(BotMap* map)
{
   _bot_map = map;
}

void BombChainReaction::compute()
{
   // clear old chain data
   _chain.clear();
   _visited.clear();

   // iterate through all bombs in case their not visited
   for (BotBombMapItem* item : _bot_map->getBombs())
   {
      if (!_visited.contains(item))
      {
         // build a detonation chain and append it in case it's not empty
         std::vector<BotBombMapItem*> items;

         iterate(item, items);

         if (!items.empty())
         {
            _chain.push_back(items);
         }
      }
   }
}

/*!
   \param item root item
   \param items list to add items to
*/
void BombChainReaction::iterate(BotBombMapItem* item, std::vector<BotBombMapItem*>& items)
{
   _visited.insert(item);
   items.push_back(item);

   // check which bombs are hit by the given bomb
   const int x = item->getX();
   const int y = item->getY();

   for (const Point& direction : _directions)
   {
      for (int i = 1; i <= item->getFlames(); i++)
      {
         const int xi = x + i * direction.x();
         const int yi = y + i * direction.y();

         if (xi >= 0 && xi < _bot_map->getWidth() && yi >= 0 && yi < _bot_map->getHeight())
         {
            MapItem* map_item = _bot_map->getItem(xi, yi);

            if (map_item)
            {
               if (map_item->getType() == MapItem::Bomb)
               {
                  auto* hit_bomb = dynamic_cast<BotBombMapItem*>(map_item);

                  if (!_visited.contains(hit_bomb))
                  {
                     iterate(hit_bomb, items);
                  }
               }

               // evaluating this direction ends here since we hit an item
               break;
            }
         }
      }
   }
}

void BombChainReaction::initDirections()
{
   _directions = {Point(0, -1), Point(0, 1), Point(-1, 0), Point(1, 0)};
}

/*!
   \return detonation chain
*/
const BombChainReaction::ChainList& BombChainReaction::getDetonationChain() const
{
   return _chain;
}
