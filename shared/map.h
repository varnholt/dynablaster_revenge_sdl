#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "point.h"

class MapItem;
class MapItemCreatedPacket;
class MapItemRemovedPacket;

class Map
{
public:
   Map(int32_t width, int32_t height);
   virtual ~Map() = default;

   void initialize();
   void initializeTestMap();

   // empty if the field is free
   [[nodiscard]] const std::shared_ptr<MapItem>& getItem(int32_t x, int32_t y) const;

   // replaces whatever was there before
   void setItem(int32_t x, int32_t y, std::shared_ptr<MapItem> item);

   // keeps owning an item that is temporarily off the grid (a kicked bomb on its way)
   void addLiftedItem(std::shared_ptr<MapItem> item);

   // hands a lifted item back, empty if there is none with that id
   [[nodiscard]] std::shared_ptr<MapItem> takeLiftedItem(int32_t unique_id);

   // check if an extra is hidden in some stone
   [[nodiscard]] bool isHiddenExtraAvailable() const;

   [[nodiscard]] int32_t getWidth() const;
   [[nodiscard]] int32_t getHeight() const;
   [[nodiscard]] int32_t getMaxPlayers() const;

   void setStartPositions(const std::vector<Point>& positions);
   [[nodiscard]] Point getStartPosition(int32_t player_number) const;

   [[nodiscard]] static std::unique_ptr<Map> generateMap(
      int32_t width,
      int32_t height,
      int32_t stone_count,
      int32_t extra_bomb_count,
      int32_t extra_flame_count,
      int32_t extra_speed_up_count,
      int32_t extra_kick_count,
      int32_t extra_skull_count,
      const std::vector<Point>& start_positions
   );

   [[nodiscard]] std::vector<std::unique_ptr<MapItemCreatedPacket>> getMapItemCreatedPackets();
   [[nodiscard]] std::vector<std::unique_ptr<MapItemRemovedPacket>> getMapItemRemovedPackets();

   // stop all ticking bombs on the map
   void stopBombs();

   [[nodiscard]] static int32_t getManhattanLength(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

   // filter points by manhattan distance to position
   [[nodiscard]] static std::vector<Point> getManhattanFiltered(
      const Point& position,
      const std::vector<Point>& points,
      int32_t manhattan_length_max,
      int32_t manhattan_length_min = std::numeric_limits<int32_t>::min()
   );

protected:
   int32_t _width = 0;
   int32_t _height = 0;

   // flattened width * height grid
   std::vector<std::shared_ptr<MapItem>> _map;

   // items currently off the grid, see addLiftedItem()
   std::vector<std::shared_ptr<MapItem>> _lifted_items;

   std::vector<Point> _start_positions;
};
