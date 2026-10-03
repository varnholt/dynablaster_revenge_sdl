#include "botmap.h"

#include <algorithm>
#include <cstdlib>
#include <format>
#include <string>
#include <vector>

#include "random.h"

// shared
#include "blockmapitem.h"
#include "bombmapitem.h"
#include "botbombmapitem.h"
#include "extramapitem.h"
#include "logging.h"
#include "mapitem.h"
#include "stonemapitem.h"

namespace
{
/*!
   \param list list to randomize
*/
template <typename T>
void randomize(std::vector<T>& list)
{
   for (int index = static_cast<int>(list.size()) - 1; index > 0; --index)
   {
      const int swap_index = Random::bounded(index + 1);
      std::swap(list[index], list[swap_index]);
   }
}
}  // namespace

BotMap::BotMap() : Map(13, 11)
{
   initDirections();
   _traversed_positions.assign(static_cast<size_t>(getWidth() * getHeight()), false);
}

/*!
   \param width map width
   \param height map height
*/
BotMap::BotMap(int width, int height) : Map(width, height)
{
   initDirections();
   _traversed_positions.assign(static_cast<size_t>(getWidth() * getHeight()), false);
}

void BotMap::initDirections()
{
   _directions.push_back(Constants::DirectionUp);
   _directions.push_back(Constants::DirectionDown);
   _directions.push_back(Constants::DirectionLeft);
   _directions.push_back(Constants::DirectionRight);

   _directions_and_current.push_back(Constants::DirectionUnknown);
   _directions_and_current.push_back(Constants::DirectionUp);
   _directions_and_current.push_back(Constants::DirectionDown);
   _directions_and_current.push_back(Constants::DirectionLeft);
   _directions_and_current.push_back(Constants::DirectionRight);

   _directions_randomized = _directions;
   randomize(_directions_randomized);
}

const std::vector<Constants::Direction>& BotMap::getDirectionsAndCurrent()
{
   return _directions_and_current;
}

/*!
   \param x x pos
   \param y y pos
   \param flames no of flames
   \param enemies enemy positions
   \return \c true if a bomb drop now could kill
*/
bool BotMap::isBombDropDeadly(int x, int y, int flames, const std::vector<Point>& enemies) const
{
   int direction_x = 0;
   int direction_y = 0;

   for (Constants::Direction direction : _directions_and_current)
   {
      switch (direction)
      {
         case Constants::DirectionUp:
            direction_x = 0;
            direction_y = -1;
            break;
         case Constants::DirectionDown:
            direction_x = 0;
            direction_y = 1;
            break;
         case Constants::DirectionLeft:
            direction_x = -1;
            direction_y = 0;
            break;
         case Constants::DirectionRight:
            direction_x = 1;
            direction_y = 0;
            break;
         default:
            direction_x = 0;
            direction_y = 0;
            break;
      }

      // example:
      //    2 flames
      //    => check x+0, y
      //    => check x+1, y
      //    => check x+2, y
      for (int i = 1; i <= flames; i++)
      {
         const int position_x = x + i * direction_x;
         const int position_y = y + i * direction_y;

         // check if position runs out of field scope
         const bool valid = position_x >= 0 && position_y >= 0 && position_x < getWidth() && position_y < getHeight();

         // bomb hit something, abort this direction
         if (!valid || getItem(position_x, position_y))
         {
            break;
         }

         // check if there's a player
         if (std::ranges::find(enemies, Point(position_x, position_y)) != enemies.end())
         {
            return true;
         }
      }
   }

   return false;
}

/*!
   \param player_id player id
   \param bomb_count player bomb count
   \return \c true if all bombs have been consumed
*/
bool BotMap::isBombAmountConsumed(int player_id, int bomb_count) const
{
   return getBombs(player_id).size() < static_cast<size_t>(bomb_count);
}

/*!
   \param player_id player id
   \return list of bombs by player id
*/
std::vector<std::reference_wrapper<BotBombMapItem>> BotMap::getBombs(int player_id) const
{
   std::vector<std::reference_wrapper<BotBombMapItem>> bombs;

   for (int y = 0; y < getHeight(); y++)
   {
      for (int x = 0; x < getWidth(); x++)
      {
         const auto& item = getItem(x, y);

         // every bomb on a bot map is a BotBombMapItem, see BotClient::processMapItemCreated()
         if (item && item->getType() == MapItem::Bomb)
         {
            auto& bomb = static_cast<BotBombMapItem&>(*item);

            if (bomb.getPlayerId() == player_id || player_id == -1)
            {
               bombs.push_back(bomb);
            }
         }
      }
   }

   return bombs;
}

/*!
   \param item map item to create
*/
void BotMap::createMapItem(const std::shared_ptr<MapItem>& item)
{
   setItem(item->getX(), item->getY(), item);
}

/*!
   \param remove_item item to remove
*/
void BotMap::removeMapItem(const MapItem& remove_item)
{
   if (getItem(remove_item.getX(), remove_item.getY()).get() == &remove_item)
   {
      setItem(remove_item.getX(), remove_item.getY(), nullptr);
   }
}

/*!
   \param x x pos to look up
   \param y y pos to look up
   \return \c true if traversed
*/
bool BotMap::isTraversed(int x, int y) const
{
   return _traversed_positions[y * getWidth() + x];
}

/*!
   \param x x pos to look up
   \param y y pos to look up
   \param traversed traversed flag
*/
void BotMap::setTraversed(int x, int y, bool traversed)
{
   _traversed_positions[y * getWidth() + x] = traversed;
}

void BotMap::resetTraversedMap()
{
   std::fill(_traversed_positions.begin(), _traversed_positions.end(), false);
}

/*!
   \param list direction list
*/
void BotMap::debugDirections(const std::vector<Constants::Direction>& list)
{
   std::string line;

   for (Constants::Direction direction : list)
   {
      switch (direction)
      {
         case Constants::DirectionUp:
            line.append("; up");
            break;
         case Constants::DirectionDown:
            line.append("; down");
            break;
         case Constants::DirectionLeft:
            line.append("; left");
            break;
         case Constants::DirectionRight:
            line.append("; right");
            break;
         default:
            break;
      }
   }

   qDebug("%s", line.c_str());
}

/*!
   \param x x position
   \param y y position
   \param iteration iteration no.
*/
void BotMap::updateReachablePositions(int x, int y, int iteration)
{
   const int current_iteration = iteration;
   iteration++;

   if (current_iteration == 0)
   {
      resetTraversedMap();
   }

   // current position is reachable
   setTraversed(x, y, true);

   for (Constants::Direction direction : _directions)
   {
      int xt = x;
      int yt = y;

      switch (direction)
      {
         case Constants::DirectionUp:
            yt--;
            break;
         case Constants::DirectionDown:
            yt++;
            break;
         case Constants::DirectionLeft:
            xt--;
            break;
         case Constants::DirectionRight:
            xt++;
            break;
         default:
            break;
      }

      // continue search if
      // - limits not exceeded
      // - position is not yet traversed
      // - there's nothing in the way
      const bool within_limits = (xt >= 0 && yt >= 0 && xt < getWidth() && yt < getHeight());

      std::shared_ptr<MapItem> item;

      if (within_limits)
      {
         item = getItem(xt, yt);
      }

      if (within_limits && !isTraversed(xt, yt) && (!item || !item->isBlocking()))
      {
         updateReachablePositions(xt, yt, iteration);
      }
   }

   if (current_iteration == 0)
   {
      _reachable_positions.clear();

      for (int xi = 0; xi < getWidth(); xi++)
      {
         for (int yi = 0; yi < getHeight(); yi++)
         {
            if (isTraversed(xi, yi))
            {
               // store reachable position
               _reachable_positions.push_back(Point(xi, yi));
            }
         }
      }
   }

   if (_reachable_positions.size() == static_cast<size_t>(getWidth() * getHeight()))
   {
      qDebug("we're fucked");
      _reachable_positions.clear();
   }
}

/*!
   \param x x position
   \param y y position
   \param iteration iteration no.
*/
void BotMap::updateReachablePositionsRandomized(int x, int y, int iteration)
{
   const int current_iteration = iteration;
   iteration++;

   if (current_iteration == 0)
   {
      resetTraversedMap();
   }

   // current position is reachable
   setTraversed(x, y, true);

   for (Constants::Direction direction : _directions_randomized)
   {
      int xt = x;
      int yt = y;

      switch (direction)
      {
         case Constants::DirectionUp:
            yt--;
            break;
         case Constants::DirectionDown:
            yt++;
            break;
         case Constants::DirectionLeft:
            xt--;
            break;
         case Constants::DirectionRight:
            xt++;
            break;
         default:
            break;
      }

      // continue search if
      // - limits not exceeded
      // - position is not yet traversed
      // - there's nothing in the way
      const bool within_limits = (xt >= 0 && yt >= 0 && xt < getWidth() && yt < getHeight());

      std::shared_ptr<MapItem> item;

      if (within_limits)
      {
         item = getItem(xt, yt);
      }

      if (within_limits && !isTraversed(xt, yt) && (!item || !item->isBlocking()))
      {
         updateReachablePositionsRandomized(xt, yt, iteration);
      }
   }

   if (current_iteration == 0)
   {
      _reachable_positions.clear();

      for (int xi = 0; xi < getWidth(); xi++)
      {
         for (int yi = 0; yi < getHeight(); yi++)
         {
            if (isTraversed(xi, yi))
            {
               // store reachable position
               _reachable_positions.push_back(Point(xi, yi));
            }
         }
      }
   }

   if (_reachable_positions.size() == static_cast<size_t>(getWidth() * getHeight()))
   {
      qDebug("we're fucked");
      _reachable_positions.clear();
   }
}

void BotMap::updateReachableExtras()
{
   _reachable_extras.clear();

   for (const Point& p : _reachable_positions)
   {
      const auto& item = getItem(p.x(), p.y());

      // store extra position if appropriate
      if (item && (item->getType() == MapItem::Extra))
      {
         // maybe distinguish between 'good' and 'bad' extras
         _reachable_extras.push_back(p);
      }
   }
}

/*!
   \return list of reachable points
*/
const std::vector<Point>& BotMap::getReachablePositions() const
{
   return _reachable_positions;
}

/*!
   \return list of reachable extras
*/
const std::vector<Point>& BotMap::getReachableExtras() const
{
   return _reachable_extras;
}

/*!
   \return list of reachable neighbor points
*/
std::vector<Point> BotMap::getReachableNeighborPositions(int x, int y) const
{
   std::vector<Point> positions;
   std::shared_ptr<MapItem> item;

   if (x > 0)
   {
      item = getItem(x - 1, y);

      if ((item && item->getType() == MapItem::Extra) || !item)
      {
         positions.push_back(Point(x - 1, y));
      }
   }

   if (x < getWidth() - 1)
   {
      item = getItem(x + 1, y);

      if ((item && item->getType() == MapItem::Extra) || !item)
      {
         positions.push_back(Point(x + 1, y));
      }
   }

   if (y > 0)
   {
      item = getItem(x, y - 1);

      if ((item && item->getType() == MapItem::Extra) || !item)
      {
         positions.push_back(Point(x, y - 1));
      }
   }

   if (y < getHeight() - 1)
   {
      item = getItem(x, y + 1);

      if ((item && item->getType() == MapItem::Extra) || !item)
      {
         positions.push_back(Point(x, y + 1));
      }
   }

   return positions;
}

/*!
   \return list of reachable neighbor points
*/
std::vector<Point> BotMap::getReachableNeighborPositionsRandomized(int x, int y) const
{
   std::vector<Point> positions;
   std::shared_ptr<MapItem> item;

   for (Constants::Direction direction : _directions_randomized)
   {
      switch (direction)
      {
         case Constants::DirectionUp:
         {
            if (y > 0)
            {
               item = getItem(x, y - 1);

               if ((item && item->getType() == MapItem::Extra) || !item)
               {
                  positions.push_back(Point(x, y - 1));
               }
            }

            break;
         }

         case Constants::DirectionDown:
         {
            if (y < getHeight() - 1)
            {
               item = getItem(x, y + 1);

               if ((item && item->getType() == MapItem::Extra) || !item)
               {
                  positions.push_back(Point(x, y + 1));
               }
            }

            break;
         }

         case Constants::DirectionLeft:
         {
            if (x > 0)
            {
               item = getItem(x - 1, y);

               if ((item && item->getType() == MapItem::Extra) || !item)
               {
                  positions.push_back(Point(x - 1, y));
               }
            }

            break;
         }

         case Constants::DirectionRight:
         {
            if (x < getWidth() - 1)
            {
               item = getItem(x + 1, y);

               if ((item && item->getType() == MapItem::Extra) || !item)
               {
                  positions.push_back(Point(x + 1, y));
               }
            }

            break;
         }

         default:
            break;
      }
   }

   return positions;
}

/*!
   \return a map with all stones that are going to be destroyed in a while
*/
std::vector<int> BotMap::getStonesToBeBombedMap()
{
   const int width = getWidth();
   std::vector<int> map(static_cast<size_t>(width * getHeight()), 0);

   const std::vector<Point> directions = {Point(0, -1), Point(0, 1), Point(-1, 0), Point(1, 0)};

   for (const BotBombMapItem& bomb : getBombs())
   {
      const int x = bomb.getX();
      const int y = bomb.getY();

      for (const Point& direction : directions)
      {
         for (int i = 1; i <= bomb.getFlames(); i++)
         {
            const int xi = x + i * direction.x();
            const int yi = y + i * direction.y();

            if (xi >= 0 && xi < width && yi >= 0 && yi < getHeight())
            {
               // we hit something
               if (const auto& item = getItem(xi, yi))
               {
                  if (item->getType() == MapItem::Stone)
                  {
                     map[yi * width + xi] = -1;
                  }

                  break;
               }
            }
         }
      }
   }

   return map;
}

/*!
   \param x x position
   \param y y position
   \param flames number of flames
   \return number of stones around current position
*/
int BotMap::getStoneCountAroundPoint(int x, int y, int flames)
{
   int count = 0;

   int direction_x = 0;
   int direction_y = 0;

   for (Constants::Direction direction : _directions)
   {
      switch (direction)
      {
         case Constants::DirectionUp:
            direction_x = 0;
            direction_y = -1;
            break;
         case Constants::DirectionDown:
            direction_x = 0;
            direction_y = 1;
            break;
         case Constants::DirectionLeft:
            direction_x = -1;
            direction_y = 0;
            break;
         case Constants::DirectionRight:
            direction_x = 1;
            direction_y = 0;
            break;
         default:
            break;
      }

      for (int i = 1; i <= flames; i++)
      {
         const int position_x = x + i * direction_x;
         const int position_y = y + i * direction_y;

         if (position_x >= 0 && position_x < getWidth() && position_y >= 0 && position_y < getHeight())
         {
            if (const auto& item = getItem(position_x, position_y))
            {
               if (item->getType() == MapItem::Stone)
               {
                  count++;
               }

               // we hit something
               break;
            }
         }
         else
         {
            // boundaries reached
            break;
         }
      }
   }

   return count;
}

/*!
   \param x x position
   \param y y position
   \param flames number of flames
   \param extras unique ids of stones containing an extra
   \return number of stones around current position
*/
int BotMap::getExtraStoneCountAroundPoint(int x, int y, int flames, const std::vector<int>& extras)
{
   int count = 0;

   int direction_x = 0;
   int direction_y = 0;

   for (Constants::Direction direction : _directions)
   {
      switch (direction)
      {
         case Constants::DirectionUp:
            direction_x = 0;
            direction_y = -1;
            break;
         case Constants::DirectionDown:
            direction_x = 0;
            direction_y = 1;
            break;
         case Constants::DirectionLeft:
            direction_x = -1;
            direction_y = 0;
            break;
         case Constants::DirectionRight:
            direction_x = 1;
            direction_y = 0;
            break;
         default:
            break;
      }

      for (int i = 1; i <= flames; i++)
      {
         const int position_x = x + i * direction_x;
         const int position_y = y + i * direction_y;

         if (position_x >= 0 && position_x < getWidth() && position_y >= 0 && position_y < getHeight())
         {
            if (const auto& item = getItem(position_x, position_y))
            {
               if (item->getType() == MapItem::Stone && std::ranges::find(extras, item->getUniqueId()) != extras.end())
               {
                  count++;
               }

               // we hit something
               break;
            }
         }
         else
         {
            // boundaries reached
            break;
         }
      }
   }

   return count;
}

void BotMap::debugTraversedMatrix()
{
   std::string joined;

   for (int yi = 0; yi < getHeight(); yi++)
   {
      std::string line = std::format("{:x} ", yi);

      for (int xi = 0; xi < getWidth(); xi++)
      {
         line.append(isTraversed(xi, yi) ? "o" : "x");
      }

      if (!joined.empty())
      {
         joined += '\n';
      }

      joined += line;
   }

   qDebug(" 0123456789abc");
   qDebug("%s\n", joined.c_str());
}

/*!
   \param x x position
   \param y y position
   \param hazardous hazardous flag
   \param abort abort flag
   \param distance distance to origin field
*/
void BotMap::checkPosition(int x, int y, bool& hazardous, bool& abort, int distance) const
{
   const auto& item = getItem(x, y);

   if (item)
   {
      switch (item->getType())
      {
         case MapItem::Bomb:
         {
            const auto& bomb = static_cast<const BombMapItem&>(*item);

            if (bomb.getFlames() >= distance)
            {
               hazardous = true;
               abort = true;
            }
            else
            {
               // todo:
               // recursion: does bomb trigger other bombs?
            }

            break;
         }

         default:
         case MapItem::Block:
         case MapItem::Extra:
         case MapItem::Stone:
         case MapItem::Unknown:
         {
            abort = true;
            break;
         }
      }
   }
}

/*!
   \param x x position
   \param y y position
   \return \c true if hazardous
*/
bool BotMap::isPositionHazardous(int x, int y) const
{
   qFatal(
      "BotMap::isPositionHazardous: do you really want to "
      "call a O(n^2) function?"
   );

   bool hazardous = false;
   bool abort = false;

   // go to the left
   for (int xi = x - 1; xi >= 0 && !abort && !hazardous; xi--)
   {
      checkPosition(xi, y, hazardous, abort, std::abs(xi - x));
   }

   // go to the right
   abort = false;
   for (int xi = x + 1; xi < getWidth() && !abort && !hazardous; xi++)
   {
      checkPosition(xi, y, hazardous, abort, std::abs(xi - x));
   }

   // go up
   abort = false;
   for (int yi = y - 1; yi >= 0 && !abort && !hazardous; yi--)
   {
      checkPosition(x, yi, hazardous, abort, std::abs(yi - y));
   }

   // go down
   abort = false;
   for (int yi = y + 1; yi < getHeight() && !abort && !hazardous; yi++)
   {
      checkPosition(x, yi, hazardous, abort, std::abs(yi - y));
   }

   // center
   abort = false;
   if (!hazardous)
   {
      checkPosition(x, y, hazardous, abort, 0);
   }

   return hazardous;
}

/*!
   \param x x position
   \param y y position
   \return \c true if blocked
*/
bool BotMap::isPositionBlocked(int x, int y) const
{
   bool blocked = false;

   if (const auto& item = getItem(x, y))
   {
      switch (item->getType())
      {
         case MapItem::Bomb:
         case MapItem::Block:
         case MapItem::Stone:
         {
            blocked = true;
            break;
         }

         default:
            break;
      }
   }

   return blocked;
}

void BotMap::debugMapItems()
{
   std::string joined;
   char c = ' ';

   for (int yi = 0; yi < getHeight(); yi++)
   {
      std::string line;

      for (int xi = 0; xi < getWidth(); xi++)
      {
         const auto& item = getItem(xi, yi);

         if (item)
         {
            switch (item->getType())
            {
               case MapItem::Block:
                  c = '#';
                  break;
               case MapItem::Bomb:
                  c = 'B';
                  break;
               case MapItem::Extra:
                  c = 'E';
                  break;
               case MapItem::Stone:
                  c = 'S';
                  break;
               case MapItem::Unknown:
               default:
                  c = ' ';
                  break;
            }
         }

         line += std::format("  {}|", item ? c : ' ');
      }

      line += std::format("{} ", yi);

      if (!joined.empty())
      {
         joined += '\n';
      }

      joined += line;
   }

   qDebug("%s", joined.c_str());
   qDebug("  0|  1|  2|  3|  4|  5|  6|  7|  8|  9| 10| 11| 12|\n");
}
