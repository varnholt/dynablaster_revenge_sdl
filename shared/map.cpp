#include "map.h"

#include "blockmapitem.h"
#include "bombmapitem.h"
#include "extramapitem.h"
#include "logging.h"
#include "mapitemcreatedpacket.h"
#include "mapitemremovedpacket.h"
#include "playerdisease.h"
#include "random.h"
#include "stonemapitem.h"

#include <algorithm>
#include <cstdlib>
#include <memory>
#include <unordered_set>

Map::Map(int32_t width, int32_t height) : _width(width), _height(height), _map(static_cast<size_t>(width) * static_cast<size_t>(height))
{
}

const std::shared_ptr<MapItem>& Map::getItem(int32_t x, int32_t y) const
{
   return _map[static_cast<size_t>(y) * static_cast<size_t>(_width) + static_cast<size_t>(x)];
}

void Map::setItem(int32_t x, int32_t y, std::shared_ptr<MapItem> item)
{
   _map[static_cast<size_t>(y) * static_cast<size_t>(_width) + static_cast<size_t>(x)] = std::move(item);
}

void Map::addLiftedItem(std::shared_ptr<MapItem> item)
{
   _lifted_items.push_back(std::move(item));
}

std::shared_ptr<MapItem> Map::takeLiftedItem(int32_t unique_id)
{
   const auto it = std::ranges::find_if(_lifted_items, [unique_id](const auto& item) { return item->getUniqueId() == unique_id; });

   if (it == _lifted_items.end())
   {
      return {};
   }

   auto item = std::move(*it);
   _lifted_items.erase(it);
   return item;
}

bool Map::isHiddenExtraAvailable() const
{
   bool available = false;

   for (int32_t x = 0; x < _width; x++)
   {
      for (int32_t y = 0; y < _height; y++)
      {
         // skip blocks, they don't need to be investigated any further
         const bool block = (x % 2 && y % 2);

         if (!block)
         {
            const auto& item = getItem(x, y);

            if (item && item->getType() == MapItem::Stone)
            {
               const auto stone = std::dynamic_pointer_cast<StoneMapItem>(item);

               if (stone && stone->hasExtraMapItem())
               {
                  available = true;
                  break;
               }
            }
         }
      }
   }

   return available;
}

void Map::initialize()
{
   for (int32_t x = 0; x < _width; x++)
   {
      for (int32_t y = 0; y < _height; y++)
      {
         if (x % 2 && y % 2)
         {
            setItem(x, y, std::make_shared<BlockMapItem>(-1, x, y));
         }
      }
   }
}

void Map::initializeTestMap()
{
   const std::vector<Point> start_positions{Point(0, 0), Point(12, 10), Point(12, 0), Point(0, 10), Point(6, 5)};

   setStartPositions(start_positions);

   for (int32_t x = 0; x < _width; x++)
   {
      for (int32_t y = 0; y < _height; y++)
      {
         // if there's a free position..
         if (!getItem(x, y))
         {
            // ..eventually place a stone
            if (Random::bounded(100) > 75)
            {
               // but keep some space around the players' start positions
               for (const Point& player_position : start_positions)
               {
                  // check the same position, right, left, bottom, top
                  if ((x != player_position.x() && y != player_position.y()) &&
                      (x + 1 != player_position.x() && y != player_position.y()) &&
                      (x - 1 != player_position.x() && y != player_position.y()) &&
                      (x != player_position.x() && y + 1 != player_position.y()) &&
                      (x != player_position.x() && y - 1 != player_position.y()))
                  {
                     setItem(x, y, std::make_shared<StoneMapItem>(-1, x, y));
                  }
               }
            }
         }
      }
   }
}

int32_t Map::getWidth() const
{
   return _width;
}

int32_t Map::getHeight() const
{
   return _height;
}

int32_t Map::getMaxPlayers() const
{
   return static_cast<int32_t>(_start_positions.size());
}

void Map::setStartPositions(const std::vector<Point>& positions)
{
   _start_positions = positions;
}

Point Map::getStartPosition(int32_t player_number) const
{
   return _start_positions.at(static_cast<size_t>(player_number));
}

std::unique_ptr<Map> Map::generateMap(
   int32_t width,
   int32_t height,
   int32_t stone_count,
   int32_t extra_bomb_count,
   int32_t extra_flame_count,
   int32_t extra_speed_up_count,
   int32_t extra_kick_count,
   int32_t extra_skull_count,
   const std::vector<Point>& start_positions
)
{
   std::unique_ptr<Map> map;

   // check if the map is actually capable of storing the given number of stones and extras
   const int32_t extra_sum = extra_bomb_count + extra_flame_count + extra_kick_count + extra_speed_up_count + extra_skull_count;

   // number of fields to leave out for the players' start positions
   int32_t free_stones_for_start_positions = 0;
   for (const Point& player_position : start_positions)
   {
      // center
      free_stones_for_start_positions++;

      // up
      if (player_position.x() >= 0 && player_position.y() - 1 >= 0 && player_position.x() < width && player_position.y() - 1 < height)
      {
         free_stones_for_start_positions++;
      }

      // down
      if (player_position.x() >= 0 && player_position.y() + 1 >= 0 && player_position.x() < width && player_position.y() + 1 < height)
      {
         free_stones_for_start_positions++;
      }

      // left
      if (player_position.x() - 1 >= 0 && player_position.y() >= 0 && player_position.x() - 1 < width && player_position.y() < height)
      {
         free_stones_for_start_positions++;
      }

      // right
      if (player_position.x() + 1 >= 0 && player_position.y() >= 0 && player_position.x() + 1 < width && player_position.y() < height)
      {
         free_stones_for_start_positions++;
      }
   }

   // the allowed number of stones equals the number of fields without the blocks and the fields
   // to leave out for the players' start positions
   const int32_t allowed_stone_count = width * height - (width - 1) / 2 + (height - 1) / 2 - free_stones_for_start_positions;

   if ((stone_count <= allowed_stone_count) && (extra_sum <= allowed_stone_count) && (extra_sum <= stone_count))
   {
      map = std::make_unique<Map>(width, height);

      // initialize that map with blocking items
      map->initialize();
      map->setStartPositions(start_positions);

      int32_t stones_placed = 0;
      int32_t extra_bomb_placed = 0;
      int32_t extra_flame_placed = 0;
      int32_t extra_kick_placed = 0;
      int32_t extra_speed_up_placed = 0;
      int32_t extra_skull_placed = 0;

      // initiate blocked positions for those players that begin between 2 blocks
      std::unordered_set<Point> blocked_positions;
      for (const Point& p : start_positions)
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

            const Point top_left(p.x() - 1, p.y() - 1);
            const Point top_right(p.x() + 1, p.y() - 1);
            const Point bottom_left(p.x() - 1, p.y() + 1);
            const Point bottom_right(p.x() + 1, p.y() + 1);

            const std::vector<Point> open_positions{top_left, top_right, bottom_left, bottom_right};

            // i want 2 ways definitely open at maximum
            for (int32_t i = 0; i < 2; i++)
            {
               const int32_t random_index = Random::bounded(4);
               blocked_positions.insert(open_positions.at(static_cast<size_t>(random_index)));
            }
         }
      }

      while (stones_placed < stone_count)
      {
         const int32_t random_x = Random::bounded(width);
         const int32_t random_y = Random::bounded(height);

         // if there's a free position..
         if (!map->getItem(random_x, random_y))
         {
            // but keep some space around the players' start positions
            bool blocks_start_position = false;

            for (const Point& player_position : start_positions)
            {
               // check the same position, right, left, bottom, top
               if ((random_x == player_position.x() && random_y == player_position.y()) ||
                   (random_x + 1 == player_position.x() && random_y == player_position.y()) ||
                   (random_x - 1 == player_position.x() && random_y == player_position.y()) ||
                   (random_x == player_position.x() && random_y + 1 == player_position.y()) ||
                   (random_x == player_position.x() && random_y - 1 == player_position.y()) ||
                   blocked_positions.contains(Point(random_x, random_y)))
               {
                  blocks_start_position = true;
               }
            }

            if (!blocks_start_position)
            {
               map->setItem(random_x, random_y, std::make_shared<StoneMapItem>(-1, random_x, random_y));
               stones_placed++;
            }
         }
      }

      // place extras
      while ((extra_bomb_placed < extra_bomb_count) || (extra_flame_placed < extra_flame_count) ||
             (extra_speed_up_placed < extra_speed_up_count) || (extra_kick_placed < extra_kick_count) ||
             (extra_skull_placed < extra_skull_count))
      {
         const int32_t random_x = Random::bounded(width);
         const int32_t random_y = Random::bounded(height);

         const auto& item = map->getItem(random_x, random_y);

         // if there's a stone that does not contain an extra yet
         if (item && item->getType() == MapItem::Stone && !static_cast<const StoneMapItem&>(*item).hasExtraMapItem())
         {
            auto& stone = static_cast<StoneMapItem&>(*item);

            if (extra_bomb_placed < extra_bomb_count)
            {
               stone.setExtraMapItem(std::make_unique<ExtraMapItem>(-1, Constants::ExtraBomb, random_x, random_y));
               extra_bomb_placed++;
            }
            else if (extra_flame_placed < extra_flame_count)
            {
               stone.setExtraMapItem(std::make_unique<ExtraMapItem>(-1, Constants::ExtraFlame, random_x, random_y));
               extra_flame_placed++;
            }
            else if (extra_speed_up_placed < extra_speed_up_count)
            {
               stone.setExtraMapItem(std::make_unique<ExtraMapItem>(-1, Constants::ExtraSpeedup, random_x, random_y));
               extra_speed_up_placed++;
            }
            else if (extra_kick_placed < extra_kick_count)
            {
               stone.setExtraMapItem(std::make_unique<ExtraMapItem>(-1, Constants::ExtraKick, random_x, random_y));
               extra_kick_placed++;
            }
            else if (extra_skull_placed < extra_skull_count)
            {
               auto extra = std::make_unique<ExtraMapItem>(-1, Constants::ExtraSkull, random_x, random_y);
               extra->setSkullFaces(PlayerDisease::generateSkullFaces());
               stone.setExtraMapItem(std::move(extra));
               extra_skull_placed++;
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

std::vector<std::unique_ptr<MapItemCreatedPacket>> Map::getMapItemCreatedPackets()
{
   std::vector<std::unique_ptr<MapItemCreatedPacket>> packets;

   for (int32_t x = 0; x < _width; x++)
   {
      for (int32_t y = 0; y < _height; y++)
      {
         if (const auto& item = getItem(x, y))
         {
            packets.push_back(std::make_unique<MapItemCreatedPacket>(*item));
         }
      }
   }

   return packets;
}

std::vector<std::unique_ptr<MapItemRemovedPacket>> Map::getMapItemRemovedPackets()
{
   std::vector<std::unique_ptr<MapItemRemovedPacket>> packets;

   for (int32_t x = 0; x < _width; x++)
   {
      for (int32_t y = 0; y < _height; y++)
      {
         if (const auto& item = getItem(x, y))
         {
            packets.push_back(std::make_unique<MapItemRemovedPacket>(*item));

            // map items may contain shadowed items
            if (item->getType() == MapItem::Bomb)
            {
               const auto& bomb = static_cast<const BombMapItem&>(*item);

               if (const auto& shadowed_item = bomb.getShadowedItem())
               {
                  packets.push_back(std::make_unique<MapItemRemovedPacket>(*shadowed_item));
               }
            }
         }
      }
   }

   return packets;
}

void Map::stopBombs()
{
   for (int32_t x = 0; x < _width; x++)
   {
      for (int32_t y = 0; y < _height; y++)
      {
         const auto& item = getItem(x, y);

         if (item && item->getType() == MapItem::Bomb)
         {
            static_cast<BombMapItem&>(*item).stopTimer();
         }
      }
   }
}

int32_t Map::getManhattanLength(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
   return std::abs(std::abs(x1) - std::abs(x2)) + std::abs(std::abs(y1) - std::abs(y2));
}

std::vector<Point> Map::getManhattanFiltered(
   const Point& position,
   const std::vector<Point>& points,
   int32_t manhattan_length_max,
   int32_t manhattan_length_min
)
{
   std::vector<Point> filtered;

   for (const Point& point : points)
   {
      const auto length = (point - position).manhattanLength();

      if (length <= manhattan_length_max && length >= manhattan_length_min)
      {
         filtered.push_back(point);
      }
   }

   return filtered;
}
