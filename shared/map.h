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
   virtual ~Map();

   void initialize();
   void initializeTestMap();

   // non-owning observer; the object is owned by whichever code path put it there (the map's own
   // construction, or server-side game logic moving items around, e.g. during a bomb kick)
   [[nodiscard]] virtual MapItem* getItem(int32_t x, int32_t y) const;

   // does not delete whatever was there before (see getItem())
   virtual void setItem(int32_t x, int32_t y, MapItem* item);

   // check if an extra is hidden in some stone
   [[nodiscard]] bool isHiddenExtraAvailable() const;

   [[nodiscard]] int32_t getWidth() const;
   [[nodiscard]] int32_t getHeight() const;
   [[nodiscard]] int32_t getMaxPlayers() const;

   void setStartPositions(const std::vector<Point>& positions);
   [[nodiscard]] Point getStartPosition(int32_t player_number) const;

   // TODO: returns an owning raw Map*, owned by the server's Game::mMap; converting it to
   // unique_ptr is a server-side ownership change
   [[nodiscard]] static Map* generateMap(
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

   // flattened width * height grid; the destructor deletes whatever is still on it
   std::vector<MapItem*> _map;

   std::vector<Point> _start_positions;
};
