// header
#include "bombchainreaction.h"

// ai
#include "botmap.h"


//-----------------------------------------------------------------------------
/*!
*/
BombChainReaction::BombChainReaction()
 : _bot_map(0)
{
}


//-----------------------------------------------------------------------------
/*!
   \param map map to set
*/
void BombChainReaction::setBotMap(BotMap *map)
{
   _bot_map = map;
}


//-----------------------------------------------------------------------------
/*!
*/
void BombChainReaction::compute()
{
   // clear old chain data
   _chain.clear();
   _visited.clear();

   // list all bombs
   std::vector<BotBombMapItem *> bombs = _bot_map->getBombs();

   // iterate through all bombs in case their not visited
   for (BotBombMapItem* item : bombs)
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


//-----------------------------------------------------------------------------
/*!
   \param item root item
   \param items list to add items to
*/
void BombChainReaction::iterate(
   BotBombMapItem *item,
   std::vector<BotBombMapItem*>& items
)
{
   _visited.insert(item);
   items.push_back(item);

   // check which bombs are hit by the given bomb
   int x = 0;
   int y = 0;
   int xi = 0;
   int yi = 0;

   x = item->getX();
   y = item->getY();

   for (const Point& dir : _directions)
   {
      for (int i = 1; i <= item->getFlames(); i++)
      {
         xi = x + i * dir.x();
         yi = y + i * dir.y();

         if (
               xi >= 0 && xi < _bot_map->getWidth()
            && yi >= 0 && yi < _bot_map->getHeight()
         )
         {
            MapItem* map_item = _bot_map->getItem(xi, yi);

            if (map_item)
            {
               if (map_item->getType() == MapItem::Bomb)
               {
                  BotBombMapItem* hit_bomb = dynamic_cast<BotBombMapItem*>(map_item);

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


//-----------------------------------------------------------------------------
/*!
*/
void BombChainReaction::unitTest1()
{
}


//-----------------------------------------------------------------------------
/*!
*/
void BombChainReaction::initDirections()
{
   _directions.clear();
   _directions.push_back(Point(0, -1));
   _directions.push_back(Point(0, 1));
   _directions.push_back(Point(-1, 0));
   _directions.push_back(Point(1, 0));
}


//-----------------------------------------------------------------------------
/*!
   \return detonation chain
*/
const BombChainReaction::ChainList &BombChainReaction::getDetonationChain()
{
   return _chain;
}

