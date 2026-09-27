// header
#include "map.h"

// map items
#include "blockmapitem.h"
#include "bombmapitem.h"
#include "extramapitem.h"
#include "stonemapitem.h"

// shared
#include "mapitemcreatedpacket.h"
#include "mapitemremovedpacket.h"
#include "playerdisease.h"

// Qt
#include "logging.h"
#include "random.h"

#include <cstdlib>
#include <memory>
#include <unordered_set>

//-----------------------------------------------------------------------------
/*!
   constructor
*/
Map::Map(int32_t w, int32_t h)
   : mWidth(w),
     mHeight(h),
     mMap(static_cast<size_t>(mWidth) * static_cast<size_t>(mHeight), nullptr)
{
}


//-----------------------------------------------------------------------------
/*!
   destructor
*/
Map::~Map()
{
   // a stone's extra (if any) is now owned by the stone itself (unique_ptr) and cleans
   // itself up automatically - no separate delete needed here
   for (MapItem* item : mMap)
   {
      delete item;
   }
}


//-----------------------------------------------------------------------------
/*!
   \return pointer to mapitem
   \param x x position
   \param y y position
*/
MapItem* Map::getItem(int32_t x, int32_t y) const
{
   return mMap[static_cast<size_t>(y) * static_cast<size_t>(mWidth) + static_cast<size_t>(x)];
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
   \param y y position
   \param mapitem
*/
void Map::setItem(int32_t x, int32_t y, MapItem* item)
{
   mMap[static_cast<size_t>(y) * static_cast<size_t>(mWidth) + static_cast<size_t>(x)] = item;
}


//-----------------------------------------------------------------------------
/*!
   \return \c true if a hidden extra is available
*/
bool Map::isHiddenExtraAvailable() const
{
   // init
   bool available = false;
   bool block = false;
   MapItem* item = 0;
   StoneMapItem* stone = 0;

   for (int x = 0; x < mWidth; x++)
   {
      for (int y = 0; y < mHeight; y++)
      {
         // skip blocks, they don't need to be investigated any further
         block = (x % 2 && y % 2);

         if (!block)
         {
            // analyze item
            item = getItem(x, y);

            if (
                  item
               && item->getType() == MapItem::Stone
            )
            {
               stone = dynamic_cast<StoneMapItem*>(item);

               if (stone)
               {
                  if (stone->getExtraMapItem())
                  {
                     available = true;
                     break;
                  }
               }
            }

         }
      }
   }

   return available;
}


//-----------------------------------------------------------------------------
/*!
*/
void Map::initialize()
{
   for (int x = 0; x < mWidth; x++)
      for (int y = 0; y < mHeight; y++)
         if (x % 2 && y % 2)
            setItem(x, y, new BlockMapItem(-1, x, y));
}


//-----------------------------------------------------------------------------
/*!
*/
void Map::initializeTestMap()
{
   std::vector<Point> startPositions{Point(0, 0), Point(12, 10), Point(12, 0), Point(0, 10), Point(6, 5)};

   setStartPositions(startPositions);

   MapItem* item = 0;
   Point playerPosition;

   for (int x = 0; x < mWidth; x++)
   {
      for (int y = 0; y < mHeight; y++)
      {
         item = getItem(x,y);

         // if there's a free position..
         if (!item)
         {
            // ..eventually place a stone
            if ((Random::bounded(100)) > 75)
            {
               // but keep some space around the players' start positions
               for (int p = 0; p < startPositions.size(); p++)
               {
                  playerPosition = startPositions.at(p);

                  // check the same position, right, left, bottom, top
                  if (
                        (x   != playerPosition.x() && y   != playerPosition.y())
                     && (x+1 != playerPosition.x() && y   != playerPosition.y())
                     && (x-1 != playerPosition.x() && y   != playerPosition.y())
                     && (x   != playerPosition.x() && y+1 != playerPosition.y())
                     && (x   != playerPosition.x() && y-1 != playerPosition.y())
                  )
                  {
                     // finally set a stone item
                     setItem(x, y, new StoneMapItem(-1, x, y));
                  }
               }
            }
         }
      }
   }
}


//-----------------------------------------------------------------------------
/*!
   \return map width
*/
int32_t Map::getWidth() const
{
   return mWidth;
}


//-----------------------------------------------------------------------------
/*!
   \return map height
*/
int32_t Map::getHeight() const
{
   return mHeight;
}


//-----------------------------------------------------------------------------
/*!
   \param max maximum player count
*/
int32_t Map::getMaxPlayers() const
{
   return static_cast<int32_t>(mStartPositions.size());
}


//-----------------------------------------------------------------------------
/*!
   \param player start positions
*/
void Map::setStartPositions(const std::vector<Point>& positions)
{
   mStartPositions = positions;
}


//-----------------------------------------------------------------------------
/*!
   \return player's start position
*/
Point Map::getStartPosition(int32_t playerNumber) const
{
   return mStartPositions.at(static_cast<size_t>(playerNumber));
}


//-----------------------------------------------------------------------------
/*!
   \return a new map generated with the given data
*/
Map* Map::generateMap(
   int32_t width,
   int32_t height,
   int32_t stoneCount,
   int32_t extraBombCount,
   int32_t extraFlameCount,
   int32_t extraSpeedUpCount,
   int32_t extraKickCount,
   int32_t extraSkullCount,
   const std::vector<Point>& startPositions
)
{
   Map* map = 0;

   // check if the map is actually capable of storing the given
   // number of stones and extras
   int extraSum =
         extraBombCount
       + extraFlameCount
       + extraKickCount
       + extraSpeedUpCount
       + extraSkullCount;

   // number of fields to leave out for the players' start positions
   int freeStonesForStartPositions = 0;
   for (int p = 0; p < startPositions.size(); p++)
   {
      Point playerPosition = startPositions.at(p);

      // center
      freeStonesForStartPositions++;

      // up
      if (
            playerPosition.x() >= 0
         && playerPosition.y() -1 >= 0
         && playerPosition.x() < width
         && playerPosition.y()- 1 < height
      )
      {
         freeStonesForStartPositions++;
      }

      // down
      if (
            playerPosition.x() >= 0
         && playerPosition.y() +1 >= 0
         && playerPosition.x() < width
         && playerPosition.y()+ 1 < height
      )
      {
         freeStonesForStartPositions++;
      }

      // left
      if (
            playerPosition.x() - 1 >= 0
         && playerPosition.y() >= 0
         && playerPosition.x() - 1 < width
         && playerPosition.y() < height
      )
      {
         freeStonesForStartPositions++;
      }

      // right
      if (
            playerPosition.x() + 1 >= 0
         && playerPosition.y() >= 0
         && playerPosition.x() + 1 < width
         && playerPosition.y() < height
      )
      {
         freeStonesForStartPositions++;
      }
   }

   // the allowed number of stones equals the  number of
   // fields without the blocks and the fields to leave out
   // for the players' start positions
   int allowedStoneCount =
        width * height
      - (width - 1)/2 + (height -1)/2
      - freeStonesForStartPositions;

   if (
         (stoneCount <= allowedStoneCount)
      && (extraSum <= allowedStoneCount)
      && (extraSum <= stoneCount)
   )
   {

      // create a new map
      map = new Map(width, height);

      // initialize that map with blocking items
      map->initialize();

      // init the player start positions
      map->setStartPositions(startPositions);

      MapItem* item = 0;
      Point playerPosition;

      int stonesPlaced = 0;
      int extraBombPlaced = 0;
      int extraFlamePlaced = 0;
      int extraKickPlaced = 0;
      int extraSpeedUpPlaced = 0;
      int extraSkullPlaced = 0;

      int randX = 0;
      int randY = 0;
      bool blocksStartPosition = false;

      // initiate blocked positions for those players that begin between 2 blocks
      std::unordered_set<Point> blockedPositions;
      for (const Point& p : startPositions)
      {
         // uneven y positions are located between 2 fixed blocks
         if (p.y() % 2 == 1)
         {
            /*
               +---+---+---+
               | 0 |   | 1 |
               +---+---+---+
               |XXX|   |XXX|
               +---+---+---+
               | 2 |   | 3 |
               +---+---+---+
            */

            Point topLeft(    p.x() - 1, p.y() - 1);
            Point topRight(   p.x() + 1, p.y() - 1);
            Point bottomLeft( p.x() - 1, p.y() + 1);
            Point bottomRight(p.x() + 1, p.y() + 1);

            std::vector<Point> openPositions{topLeft, topRight, bottomLeft, bottomRight};

            // i want 2 ways definitely open at maximum
            for (int i = 0; i < 2; i++)
            {
               int randIndex = Random::bounded(4);

               blockedPositions.insert(openPositions.at(randIndex));
            }
         }
      }

      while (stonesPlaced < stoneCount)
      {
         randX = Random::bounded(width);
         randY = Random::bounded(height);

         item = map->getItem(randX, randY);

         // if there's a free position..
         if (!item)
         {
            // but keep some space around the players' start positions
            blocksStartPosition = false;

            for (int p = 0; p < startPositions.size(); p++)
            {
               playerPosition = startPositions.at(p);

               // check the same position, right, left, bottom, top
               if (
                     (randX   == playerPosition.x() && randY   == playerPosition.y())

                  || (randX+1 == playerPosition.x() && randY   == playerPosition.y())
                  || (randX-1 == playerPosition.x() && randY   == playerPosition.y())
                  || (randX   == playerPosition.x() && randY+1 == playerPosition.y())
                  || (randX   == playerPosition.x() && randY-1 == playerPosition.y())

                  /*
                  || (randX+1 == playerPosition.x() && randY-1 == playerPosition.y())
                  || (randX-1 == playerPosition.x() && randY-1 == playerPosition.y())
                  || (randX+1 == playerPosition.x() && randY+1 == playerPosition.y())
                  || (randX-1 == playerPosition.x() && randY+1 == playerPosition.y())
                  */

                  || blockedPositions.contains(Point(randX, randY))
               )
               {
                  blocksStartPosition = true;
               }
            }

            if (!blocksStartPosition)
            {
               // finally set a stone item
               map->setItem(
                  randX,
                  randY,
                  new StoneMapItem(
                     -1,
                     randX,
                     randY
                  )
               );

               stonesPlaced++;
            }
         }
      }

      // place extras
      while (
            (extraBombPlaced    < extraBombCount)
         || (extraFlamePlaced   < extraFlameCount)
         || (extraSpeedUpPlaced < extraSpeedUpCount)
         || (extraKickPlaced    < extraKickCount)
         || (extraSkullPlaced   < extraSkullCount)
      )
      {
         randX = Random::bounded(width);
         randY = Random::bounded(height);

         item = map->getItem(randX, randY);

         // if there's a stone that does not contain an extra yet
         if (
               item
            && item->getType() == MapItem::Stone
            && !(static_cast<StoneMapItem*>(item))->getExtraMapItem()
         )
         {
            // create bomb extras
            if (extraBombPlaced < extraBombCount)
            {
               (static_cast<StoneMapItem*>(item))->setExtraMapItem(
                  std::make_unique<ExtraMapItem>(
                     -1,
                     Constants::ExtraBomb,
                     randX,
                     randY
                  )
               );

               extraBombPlaced++;
            }

            // create flame extras
            else if (extraFlamePlaced < extraFlameCount)
            {
               (static_cast<StoneMapItem*>(item))->setExtraMapItem(
                  std::make_unique<ExtraMapItem>(
                     -1,
                     Constants::ExtraFlame,
                     randX,
                     randY
                  )
               );

               extraFlamePlaced++;
            }

            // create speedup extras
            else if (extraSpeedUpPlaced < extraSpeedUpCount)
            {
               (static_cast<StoneMapItem*>(item))->setExtraMapItem(
                  std::make_unique<ExtraMapItem>(
                     -1,
                     Constants::ExtraSpeedup,
                     randX,
                     randY
                  )
               );

               extraSpeedUpPlaced++;
            }

            // create kick extras
            else if (extraKickPlaced < extraKickCount)
            {
               (static_cast<StoneMapItem*>(item))->setExtraMapItem(
                  std::make_unique<ExtraMapItem>(
                     -1,
                     Constants::ExtraKick,
                     randX,
                     randY
                  )
               );

               extraKickPlaced++;
            }

            // create skull extras
            else if (extraSkullPlaced < extraSkullCount)
            {
               auto extra = std::make_unique<ExtraMapItem>(
                  -1,
                  Constants::ExtraSkull,
                  randX,
                  randY
               );

               // generate skull faces and init start time
               extra->setSkullFaces(PlayerDisease::generateSkullFaces());

               (static_cast<StoneMapItem*>(item))->setExtraMapItem(std::move(extra));

               extraSkullPlaced++;
            }
         }
      }
   }
   else
   {
      qWarning("Map::generateMap: given map seems to be stoned.");
   }

   return map;
}


//-----------------------------------------------------------------------------
/*!
   \param socket socket to send map to
*/
std::vector<std::unique_ptr<MapItemCreatedPacket>> Map::getMapItemCreatedPackets()
{
   std::vector<std::unique_ptr<MapItemCreatedPacket>> packets;

   MapItem* item = 0;

   for (int x = 0; x < mWidth; x++)
   {
      for (int y = 0; y < mHeight; y++)
      {
         item = getItem(x, y);

         if (item)
         {
            packets.push_back(std::make_unique<MapItemCreatedPacket>(item));
         }
      }
   }

   return packets;
}


//-----------------------------------------------------------------------------
/*!
   \param socket socket to send map to
*/
std::vector<std::unique_ptr<MapItemRemovedPacket>> Map::getMapItemRemovedPackets()
{
   std::vector<std::unique_ptr<MapItemRemovedPacket>> packets;

   MapItem* item = 0;

   for (int x = 0; x < mWidth; x++)
   {
      for (int y = 0; y < mHeight; y++)
      {
         item = getItem(x, y);

         if (item)
         {
            packets.push_back(std::make_unique<MapItemRemovedPacket>(item));

            // map items may contain shadowed items
            if (item->getType() == MapItem::Bomb)
            {
               BombMapItem* bomb = static_cast<BombMapItem*>(item);

               if (bomb->getShadowedItem())
               {
                  packets.push_back(std::make_unique<MapItemRemovedPacket>(bomb->getShadowedItem()));
               }
            }
         }
      }
   }

   return packets;
}


//-----------------------------------------------------------------------------
/*!
*/
void Map::stopBombs()
{
   MapItem* item = 0;

   for (int x = 0; x < mWidth; x++)
   {
      for (int y = 0; y < mHeight; y++)
      {
         item = getItem(x, y);

         if (item)
         {
            if (item->getType() == MapItem::Bomb)
            {
               static_cast<BombMapItem*>(item)->stopTimer();
            }
         }
      }
   }
}


//-----------------------------------------------------------------------------
/*!
   \param x1 point a x
   \param y1 point a y
   \param x2 point b x
   \param y2 point b y
   \return manhattan length between point a and b
*/
int32_t Map::getManhattanLength(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
   return std::abs(std::abs(x1) - std::abs(x2)) + std::abs(std::abs(y1) - std::abs(y2));
}


//-----------------------------------------------------------------------------
/*!
   \param pos start pos
   \param points points to check
   \param manhattanLength maximum manhattan length
   \return filtered list of points
*/
std::vector<Point> Map::getManhattanFiltered(
   const Point &pos,
   const std::vector<Point>& points,
   int32_t manhattanLengthMax,
   int32_t manhattanLengthMin
)
{
   std::vector<Point> filtered;

   Point diff;
   for (const Point& p : points)
   {
      diff = p - pos;

      if (
            diff.manhattanLength() <= manhattanLengthMax
         && diff.manhattanLength() >= manhattanLengthMin
      )
      {
         filtered.push_back(p);
      }
   }

   return filtered;
}


