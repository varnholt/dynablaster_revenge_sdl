#include "botmap.h"
#include <cstring>

#include <format>
#include <string>
#include <vector>

#include "random.h"

// shared
#include "blockmapitem.h"
#include "bombmapitem.h"
#include "botbombmapitem.h"
#include "extramapitem.h"
#include "mapitem.h"
#include "stonemapitem.h"

// Qt
#include "logging.h"

#include <algorithm>


//-----------------------------------------------------------------------------
/*!
   \param parent parent node
*/
BotMap::BotMap()
   : Map(13, 11)
{
   initDirections();
   mTraversedPositions = new bool[mWidth * mHeight];
   resetTraversedMap();
}


//-----------------------------------------------------------------------------
/*!
   \param width map width
   \param height map height
*/
BotMap::BotMap(int width, int height)
  : Map(width, height)
{
   initDirections();
   mTraversedPositions = new bool[mWidth * mHeight];
   resetTraversedMap();
}


//-----------------------------------------------------------------------------
/*!
*/
BotMap::~BotMap()
{
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \return map item at x,y
*/
MapItem *BotMap::getItem(int x, int y) const
{
   return Map::getItem(x, y);
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \param item item to set
*/
void BotMap::setItem(int x, int y, MapItem * item)
{
   Map::setItem(x, y, item);
}


//-----------------------------------------------------------------------------
/*!
   \param list list to randomize
*/
template<typename T> void randomize(std::vector<T>& list)
{
   for (int index = static_cast<int>(list.size()) - 1; index > 0; --index)
   {
      const int swapIndex = Random::bounded(index + 1);
      std::swap(list[index], list[swapIndex]);
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void BotMap::initDirections()
{
   mDirections.push_back(Constants::DirectionUp);
   mDirections.push_back(Constants::DirectionDown);
   mDirections.push_back(Constants::DirectionLeft);
   mDirections.push_back(Constants::DirectionRight);

   mDirectionsAndCurrent.push_back(Constants::DirectionUnknown);
   mDirectionsAndCurrent.push_back(Constants::DirectionUp);
   mDirectionsAndCurrent.push_back(Constants::DirectionDown);
   mDirectionsAndCurrent.push_back(Constants::DirectionLeft);
   mDirectionsAndCurrent.push_back(Constants::DirectionRight);

   mDirectionsRandomized = mDirections;
   randomize(mDirectionsRandomized);
   // debugDirections(mDirectionsRandomized);
}



//-----------------------------------------------------------------------------
/*!
*/
const std::vector<Constants::Direction>& BotMap::getDirectionsAndCurrent()
{
   return mDirectionsAndCurrent;
}


//-----------------------------------------------------------------------------
/*!
   \param x x pos
   \param y y pos
   \param flames no of flames
   \param enemies enemy positions
   \return \c true if a bomb drop now could kill
*/
bool BotMap::isBombDropDeadly(
   int x,
   int y,
   int flames,
   const std::vector<Point>& enemies
) const
{
   bool deadly = false;

   int dirX = 0;
   int dirY = 0;

   for (Constants::Direction dir : mDirectionsAndCurrent)
   {
      switch (dir)
      {
         case Constants::DirectionUp:
            dirX = 0;
            dirY = -1;
            break;
         case Constants::DirectionDown:
            dirX = 0;
            dirY = 1;
            break;
         case Constants::DirectionLeft:
            dirX = -1;
            dirY = 0;
            break;
         case Constants::DirectionRight:
            dirX = 1;
            dirY = 0;
            break;
         default:
            dirX = 0;
            dirY = 0;
            break;
      }

      // example:
      //    2 flames
      //    => check x+0, y
      //    => check x+1, y
      //    => check x+2, y
      for (int i = 1; i <= flames; i++)
      {
         int posX = x + i * dirX;
         int posY = y + i * dirY;

         // check if position runs out of field scope
         bool valid =
               posX >= 0
            && posY >= 0
            && posX < getWidth()
            && posY < getHeight();

         // bomb hit something, abort this direction
         if (
               !valid
            || getItem(posX, posY)
         )
         {
            break;
         }
         else
         {
            // check if there's a player
            if (std::find(enemies.begin(), enemies.end(), Point(posX, posY)) != enemies.end())
            {
               // we'll just leave here with a positive result
               return true;
            }
         }
      }
   }

   return deadly;
}


//-----------------------------------------------------------------------------
/*!
   \param playerId player id
   \param player bomb count
   \return \c true if all bombs have been consumed
*/
bool BotMap::isBombAmountConsumed(int playerId, int bombCount) const
{
   return getBombs(playerId).size() < bombCount;
}


//-----------------------------------------------------------------------------
/*!
   \param playerId player id
   \return list of bombs by player id
*/
std::vector<BotBombMapItem *> BotMap::getBombs(int playerId) const
{
   std::vector<BotBombMapItem *> bombs;
   MapItem* item = 0;
   BotBombMapItem* bomb = 0;

   for (int y = 0; y < getHeight(); y++)
   {
      for (int x = 0; x < getWidth(); x++)
      {
         item = getItem(x, y);

         bomb = dynamic_cast<BotBombMapItem*>(item);

         if (bomb && (bomb->getPlayerId() == playerId || playerId == -1))
            bombs.push_back(bomb);
      }
   }

   return bombs;
}


//-----------------------------------------------------------------------------
/*!
   \parem item map item to create
*/
void BotMap::createMapItem(MapItem *item)
{
   setItem(
      item->getX(),
      item->getY(),
      item
   );

   if (item->getType() == MapItem::Extra)
   {
      ExtraMapItem* extra = (ExtraMapItem*)item;
      std::string extraName;

      switch (extra->getExtraType())
      {
         case Constants::ExtraBomb:
            extraName = "bomb";
            break;
         case Constants::ExtraFlame:
            extraName = "flame";
            break;
         case Constants::ExtraSpeedup:
            extraName = "speedup";
            break;
         case Constants::ExtraKick:
            extraName = "kick";
            break;
         case Constants::ExtraSkull:
            extraName = "skull";
            break;
         default:
            extraName = "unknown";
            break;
      }

      /*
      qDebug(
         "BotMap::createMapItem: '%s' (ptr: %p) created at (%d, %d) (mapptr: %p)",
         qPrintable(extraName),
         item,
         item->getX(),
         item->getY(),
         this
      );
      */
   }
}


//-----------------------------------------------------------------------------
/*!
   \parem item item to remove
*/
void BotMap::removeMapItem(MapItem *removeItem)
{
   MapItem* foundItem = getItem(removeItem->getX(), removeItem->getY());

   if (foundItem == removeItem)
   {
      setItem(
         removeItem->getX(),
         removeItem->getY(),
         0
      );
   }
}


//-----------------------------------------------------------------------------
/*!
   \param x x pos to look up
   \param y y pos to look up
   \return \c true if traversed
*/
bool BotMap::isTraversed(int x, int y) const
{
   return mTraversedPositions[y * mWidth + x];
}


//-----------------------------------------------------------------------------
/*!
   \param x x pos to look up
   \param y y pos to look up
   \param traversed traversed flag
*/
void BotMap::setTraversed(int x, int y, bool traversed)
{
   mTraversedPositions[y * mWidth + x] = traversed;
}


//-----------------------------------------------------------------------------
/*!
*/
void BotMap::resetTraversedMap()
{
   for (int y = 0; y < getHeight(); y++)
      for (int x = 0; x < getWidth(); x++)
         setTraversed(x, y, false);
}


//-----------------------------------------------------------------------------
/*!
   \param list direction list
*/
void BotMap::debugDirections(const std::vector<Constants::Direction> &list)
{
   std::string line;

   for (Constants::Direction dir : list)
   {
      switch (dir)
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


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \parem iteration no.
*/
void BotMap::updateReachablePositions(int x, int y, int iteration)
{
   int currentIteration = iteration;
   iteration++;

   if (currentIteration == 0)
   {
      resetTraversedMap();
   }

   // current position is reachable
   setTraversed(x, y, true);

   int xt = 0;
   int yt = 0;

   for (Constants::Direction dir : mDirections)
   {
      xt = x;
      yt = y;

      switch (dir)
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
      bool withinLimits = (
               xt >= 0
            && yt >= 0
            && xt < getWidth()
            && yt < getHeight()
         );

      MapItem* item = 0;

      if (withinLimits)
         item = getItem(xt, yt);

      if (
             withinLimits
         && !isTraversed(xt, yt)
         && (
               !item
            || (item && !item->isBlocking())
        )
      )
      {
         updateReachablePositions(xt, yt, iteration);
      }
   }

   if (currentIteration == 0)
   {
      mReachablePositions.clear();

      for (int xi = 0; xi < getWidth(); xi++)
      {
         for (int yi = 0; yi < getHeight(); yi++)
         {
            if (isTraversed(xi, yi))
            {
               // store reachable position
               mReachablePositions.push_back(Point(xi, yi));
            }
         }
      }
   }

   if (mReachablePositions.size() == static_cast<size_t>(mWidth * mHeight))
   {
      qDebug("we're fucked");
      mReachablePositions.clear();
   }
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \parem iteration no.
*/
void BotMap::updateReachablePositionsRandomized(int x, int y, int iteration)
{
   int currentIteration = iteration;
   iteration++;

   if (currentIteration == 0)
   {
      resetTraversedMap();
   }

   // current position is reachable
   setTraversed(x, y, true);

   int xt = 0;
   int yt = 0;

   for (Constants::Direction dir : mDirectionsRandomized)
   {
      xt = x;
      yt = y;

      switch (dir)
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
      bool withinLimits = (
               xt >= 0
            && yt >= 0
            && xt < getWidth()
            && yt < getHeight()
         );

      MapItem* item = 0;

      if (withinLimits)
         item = getItem(xt, yt);

      if (
             withinLimits
         && !isTraversed(xt, yt)
         && (
               !item
            || (item && !item->isBlocking())
        )
      )
      {
         updateReachablePositionsRandomized(xt, yt, iteration);
      }
   }

   if (currentIteration == 0)
   {
      mReachablePositions.clear();

      for (int xi = 0; xi < getWidth(); xi++)
      {
         for (int yi = 0; yi < getHeight(); yi++)
         {
            if (isTraversed(xi, yi))
            {
               // store reachable position
               mReachablePositions.push_back(Point(xi, yi));
            }
         }
      }
   }

   if (mReachablePositions.size() == static_cast<size_t>(mWidth * mHeight))
   {
      qDebug("we're fucked");
      mReachablePositions.clear();
   }
}


//-----------------------------------------------------------------------------
/*!
*/
void BotMap::updateReachableExtras()
{
   mReachableExtras.clear();

   int x = 0;
   int y = 0;
   MapItem* item = 0;
   for (const Point& p : mReachablePositions)
   {
      x = p.x();
      y = p.y();

      item = getItem(x, y);

      // store extra position if appropriate
      if (
             item
         && (item->getType() == MapItem::Extra)
      )
      {
         // maybe distinguish between 'good' and 'bad' extras
         mReachableExtras.push_back(p);
      }
   }
}


//-----------------------------------------------------------------------------
/*!
   \return list of reachable points
*/
const std::vector<Point>& BotMap::getReachablePositions() const
{
   return mReachablePositions;
}


//-----------------------------------------------------------------------------
/*!
   \return list of reachable extras
*/
const std::vector<Point>& BotMap::getReachableExtras() const
{
   return mReachableExtras;
}


//-----------------------------------------------------------------------------
/*!
   \return list of reachable neighbor points
*/
std::vector<Point> BotMap::getReachableNeighborPositions(int x, int y) const
{
   std::vector<Point> positions;
   MapItem* item = 0;

   if (x > 0)
   {
      item = getItem(x - 1, y);

      if (
             (item && item->getType() == MapItem::Extra)
          || !item
      )
      {
         positions.push_back(Point(x - 1, y));
      }
   }

   if (x < getWidth() - 1)
   {
      item = getItem(x + 1, y);

      if (
             (item && item->getType() == MapItem::Extra)
          || !item
      )
      {
         positions.push_back(Point(x + 1, y));
      }
   }

   if (y > 0)
   {
      item = getItem(x, y - 1);

      if (
             (item && item->getType() == MapItem::Extra)
          || !item
      )
      {
         positions.push_back(Point(x, y - 1));
      }
   }

   if (y < getHeight() - 1)
   {
      item = getItem(x, y + 1);

      if (
             (item && item->getType() == MapItem::Extra)
          || !item
      )
      {
         positions.push_back(Point(x, y + 1));
      }
   }

   return positions;
}


//-----------------------------------------------------------------------------
/*!
   \return list of reachable neighbor points
*/
std::vector<Point> BotMap::getReachableNeighborPositionsRandomized(int x, int y) const
{
   std::vector<Point> positions;
   MapItem* item = 0;

   for (Constants::Direction dir : mDirectionsRandomized)
   {
      switch (dir)
      {
         case Constants::DirectionUp:
         {
            if (y > 0)
            {
               item = getItem(x, y - 1);

               if (
                      (item && item->getType() == MapItem::Extra)
                   || !item
               )
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

               if (
                      (item && item->getType() == MapItem::Extra)
                   || !item
               )
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

               if (
                      (item && item->getType() == MapItem::Extra)
                   || !item
               )
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

               if (
                      (item && item->getType() == MapItem::Extra)
                   || !item
               )
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



//-----------------------------------------------------------------------------
/*!
   \return a map with all stones that are going to be destroyed in a while
*/
int* BotMap::getStonesToBeBombedMap()
{
   int dim = getWidth() * getHeight();
   int* map = new int[dim];
   std::memset(map, 0, dim * sizeof(int));

   std::vector<BotBombMapItem *> bombs = getBombs();

   std::vector<Point> dirs;
   dirs.push_back(Point(0, -1));
   dirs.push_back(Point(0, 1));
   dirs.push_back(Point(-1, 0));
   dirs.push_back(Point(1, 0));

   int x = 0;
   int y = 0;
   int xi = 0;
   int yi = 0;

   MapItem* item = 0;
   for (BotBombMapItem* bomb : bombs)
   {
      x = bomb->getX();
      y = bomb->getY();

      for (const Point& dir : dirs)
      {
         for (int i = 1; i <= bomb->getFlames(); i++)
         {
            xi = x + i * dir.x();
            yi = y + i * dir.y();

            if (
                  xi >= 0 && xi < getWidth()
               && yi >= 0 && yi < getHeight()
            )
            {
               item = getItem(xi, yi);

               // we hit something
               if (item)
               {
                  if (item->getType() == MapItem::Stone)
                     map[yi * getWidth() + xi] = -1;

                  break;
               }
            }
         }
      }
   }

   return map;
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \param flames number of flames
   \return number of stones around current position
*/
int BotMap::getStoneCountAroundPoint(int x, int y, int flames)
{
   int count = 0;
   MapItem* item = 0;

   int dirX = 0;
   int dirY = 0;

   for (Constants::Direction dir : mDirections)
   {
      switch (dir)
      {
         case Constants::DirectionUp:
            dirX = 0;
            dirY = -1;
            break;
         case Constants::DirectionDown:
            dirX = 0;
            dirY = 1;
            break;
         case Constants::DirectionLeft:
            dirX = -1;
            dirY = 0;
            break;
         case Constants::DirectionRight:
            dirX = 1;
            dirY = 0;
            break;
         default:
            break;
      }

      for (int i = 1; i <= flames; i++)
      {
         int posX = x + i * dirX;
         int posY = y + i * dirY;

         if (
               posX >= 0 && posX < getWidth()
            && posY >= 0 && posY < getHeight()
         )
         {
            item = getItem(posX, posY);

            if (item)
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


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \param flames number of flames
   \return number of stones around current position
*/
int BotMap::getExtraStoneCountAroundPoint(
   int x,
   int y,
   int flames,
   const std::vector<int>& extras
)
{
   int count = 0;
   MapItem* item = 0;

   int dirX = 0;
   int dirY = 0;

   for (Constants::Direction dir : mDirections)
   {
      switch (dir)
      {
         case Constants::DirectionUp:
            dirX = 0;
            dirY = -1;
            break;
         case Constants::DirectionDown:
            dirX = 0;
            dirY = 1;
            break;
         case Constants::DirectionLeft:
            dirX = -1;
            dirY = 0;
            break;
         case Constants::DirectionRight:
            dirX = 1;
            dirY = 0;
            break;
         default:
            break;
      }

      for (int i = 1; i <= flames; i++)
      {
         int posX = x + i * dirX;
         int posY = y + i * dirY;

         if (
               posX >= 0 && posX < getWidth()
            && posY >= 0 && posY < getHeight()
         )
         {
            item = getItem(posX, posY);

            if (item)
            {
               if (item->getType() == MapItem::Stone)
               {
                  if (std::find(extras.begin(), extras.end(), item->getUniqueId()) != extras.end())
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


//-----------------------------------------------------------------------------
/*!
*/
void BotMap::debugTraversedMatrix()
{
   std::vector<std::string> lines;

   for (int yi = 0; yi < getHeight(); yi++)
   {
      std::string line = std::format("{:x} ", yi);

      for (int xi = 0; xi < getWidth(); xi++)
      {
         line.append(
            isTraversed(xi, yi) ? "o" : "x"
         );
      }

      lines.push_back(line);
   }

   std::string joined;
   for (const auto& line : lines)
   {
      if (!joined.empty())
         joined += '\n';
      joined += line;
   }

   qDebug(" 0123456789abc");
   qDebug("%s\n", joined.c_str());
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \param hazardous hazardous flag
   \param abort abort flag
   \param distance distance to origin field
*/
void BotMap::checkPosition(
   int x,
   int y,
   bool& hazardous,
   bool& abort,
   int distance
) const
{
   MapItem* item = getItem(x, y);
   BombMapItem* bomb = 0;

   if (item)
   {
      switch (item->getType())
      {
         case MapItem::Bomb:
         {
            bomb = (BombMapItem*)item;

            // if (true)
            if (bomb->getFlames() >= distance)
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


//-----------------------------------------------------------------------------
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
   int distance = 0;

   // go to the left
   for (int xi = x - 1; xi >= 0 && !abort && !hazardous; xi--)
   {
      distance = abs(xi - x);
      checkPosition(xi, y, hazardous, abort, distance);
   }

   // go to the right
   abort = false;
   for (int xi = x + 1; xi < getWidth() && !abort && !hazardous; xi++)
   {
      distance = abs(xi - x);
      checkPosition(xi, y, hazardous, abort, distance);
   }

   // go up
   abort = false;
   for (int yi = y - 1; yi >= 0 && !abort && !hazardous; yi--)
   {
      distance = abs(yi - y);
      checkPosition(x, yi, hazardous, abort, distance);
   }

   // go down
   abort = false;
   for (int yi = y + 1; yi < getHeight() && !abort && !hazardous; yi++)
   {
      distance = abs(yi - y);
      checkPosition(x, yi, hazardous, abort, distance);
   }

   // center
   abort = false;
   if (!hazardous)
   {
      checkPosition(x, y, hazardous, abort, 0);
   }

   return hazardous;
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \return \c true if blocked
*/
bool BotMap::isPositionBlocked(int x, int y) const
{
   bool blocked = false;

   MapItem* item = getItem(x, y);

   if (item)
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


//-----------------------------------------------------------------------------
/*!
*/
void BotMap::debugMapItems()
{
   std::vector<std::string> lines;
   MapItem* item = 0;
   char c = ' ';

   for (int yi = 0; yi < getHeight(); yi++)
   {
      std::string line;

      for (int xi = 0; xi < getWidth(); xi++)
      {
         item = getItem(xi, yi);

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

      lines.push_back(line);
   }

   std::string joined;
   for (const auto& l : lines)
   {
      if (!joined.empty())
         joined += '\n';
      joined += l;
   }

   qDebug("%s", joined.c_str());
   qDebug("  0|  1|  2|  3|  4|  5|  6|  7|  8|  9| 10| 11| 12|\n");
}


