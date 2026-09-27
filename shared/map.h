#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <vector>

#include "point.h"

// forward declarations
class MapItem;
class MapItemCreatedPacket;
class MapItemRemovedPacket;


class Map
{

   public:

      //! constructor
      Map(
         int32_t width,
         int32_t height
      );

      //! destructor
      virtual ~Map();

      //! init map
      void initialize();

      //! init test map
      void initializeTestMap();

      //! get item at x, y - non-owning observer; whatever object it points to is still owned
      //! by whichever code path put it there (the map's own construction, or server-side game
      //! logic moving MapItem*s around, e.g. while a bomb is mid-kick-animation)
      [[nodiscard]] virtual MapItem* getItem(
         int32_t x,
         int32_t y
      ) const;

      //! set item at x, y - does not delete whatever was there before (see getItem's comment);
      //! that mixed ownership model is a pre-existing design trait, not something this pass
      //! changes - only the grid's own storage container was modernized
      virtual void setItem(
         int32_t x,
         int32_t y,
         MapItem*
      );

      //! check if an extra is hidden in some stone
      [[nodiscard]] bool isHiddenExtraAvailable() const;

      //! getter for the field's width
      [[nodiscard]] int32_t getWidth() const;

      //! getter for the field's height
      [[nodiscard]] int32_t getHeight() const;

      //! getter for the maximum player count
      [[nodiscard]] int32_t getMaxPlayers() const;

      //! setter for the player start positions
      void setStartPositions(const std::vector<Point>& positions);

      //! get the start position for the given player id
      [[nodiscard]] Point getStartPosition(int32_t playerNumber) const;

      //! generate a whole map
      //!
      //! TODO: returns an owning raw Map* - the sole real owner is Game::mMap (server/game.h,
      //! also a raw Map*, deleted in ~Game() and reassigned after an explicit delete elsewhere),
      //! with several other server-side classes (ExtraSpawn, CollisionDetection,
      //! BombKickAnimation) holding their own non-owning Map* observers alongside it. Converting
      //! this to std::unique_ptr<Map> is a server/-side ownership change (Game::mMap's type, both
      //! delete sites, and every observer that receives the pointer through Game), not a
      //! shared/-only fix - deferred to the client/server modernization pass.
      [[nodiscard]] static Map* generateMap(
         int32_t width,
         int32_t height,
         int32_t stoneCount,
         int32_t extraBombCount,
         int32_t extraFlameCount,
         int32_t extraSpeedUpCount,
         int32_t extraKickCount,
         int32_t extraSkullCount,
         const std::vector<Point>& startPositions
      );

      //! get a list of mapitem-created-packets for the current map
      [[nodiscard]] std::vector<std::unique_ptr<MapItemCreatedPacket>> getMapItemCreatedPackets();

      //! get a list of mapitem-removed-packets for the current map
      [[nodiscard]] std::vector<std::unique_ptr<MapItemRemovedPacket>> getMapItemRemovedPackets();

      //! stop all ticking bombs on the map
      void stopBombs();

      //! calculate manhattan length between 2 points
      [[nodiscard]] static int32_t getManhattanLength(int32_t x1, int32_t y1, int32_t x2, int32_t y2);

      //! filter list of points by given maximum manhattan length
      [[nodiscard]] static std::vector<Point> getManhattanFiltered(
         const Point& pos,
         const std::vector<Point>&,
         int32_t manhattanLengthMax,
         int32_t manhattanLengthMin = std::numeric_limits<int32_t>::min()
      );


   protected:

      //! width of the map
      int32_t mWidth;

      //! height of the map
      int32_t mHeight;

      //! the map itself - a flattened width*height grid of non-owning MapItem* observers (see
      //! getItem()/setItem()'s comments); std::vector replaces what used to be a manually
      //! new[]-allocated MapItem** that was never delete[]-ed (a real leak of the outer array,
      //! fixed for free by this container change - only the per-element pointees still need the
      //! explicit delete loop in the destructor, since the vector itself doesn't own them)
      std::vector<MapItem*> mMap;

      //! start positions
      std::vector<Point> mStartPositions;
};

