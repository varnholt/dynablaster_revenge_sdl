#ifndef BOTMAP_H
#define BOTMAP_H

// base
#include "map.h"

// shared
#include "constants.h"

#include <vector>

// forward declarations
class BotBombMapItem;

class BotMap : public Map
{
public:
   //! constructors
   BotMap(int width, int height);

   //! update reachable positions
   void updateReachablePositions(int x, int y, int iteration = 0);

   //! update reachable positions
   void updateReachablePositionsRandomized(int x, int y, int iteration = 0);

   //! update list of reachable extras, to be called after updateReachablePositions(...)
   void updateReachableExtras();

   //! getter for reachable positions
   const std::vector<Point>& getReachablePositions() const;

   //! getter for reachable extras
   const std::vector<Point>& getReachableExtras() const;

   //! getter for reachable neighbor positions
   std::vector<Point> getReachableNeighborPositions(int x, int y) const;

   //! getter for reachable neighbor positions in randomized form
   std::vector<Point> getReachableNeighborPositionsRandomized(int x, int y) const;

   //! number of stones neighbored to the given position
   int getStoneCountAroundPoint(int x, int y, int flame_count);

   //! number of extras within stones neighbored to the given position
   int getExtraStoneCountAroundPoint(int x, int y, int flame_count, const std::vector<int>& extras);

   //! returns a map with all stones that will be destroyed soon
   std::vector<int> getStonesToBeBombedMap();

   //! debug output reachable positions
   void debugTraversedMatrix();

   //! debug output map items
   void debugMapItems();

   //! check if given position is dangerous
   bool isPositionHazardous(int x, int y) const;

   //! check if a position is blocked by a stone, block or bomb
   bool isPositionBlocked(int x, int y) const;

   //! check a single position
   void checkPosition(int x, int y, bool& hazardous, bool& abort, int distance) const;

   //! initialize player directions
   void initDirections();

   //! check if a bomb drop could kill
   bool isBombDropDeadly(int x, int y, int flames, const std::vector<Point>& enemies) const;

   //! get bombs placed by player id
   std::vector<BotBombMapItem*> getBombs(int player_id = -1) const;

   //! check if player has consumed all its pots
   bool isBombAmountConsumed(int player_id, int bomb_count) const;

   //! getter for directions and current list
   const std::vector<Constants::Direction>& getDirectionsAndCurrent();

   //! a map item has been created
   void createMapItem(MapItem* item);

   //! a map item has been removed
   void removeMapItem(MapItem* remove_item);

protected:
   //! constructors
   BotMap();

   //! get traversed flag for x, y
   bool isTraversed(int x, int y) const;

   //! set traversed flag at x, y
   void setTraversed(int x, int y, bool);

   //! reset traversed map
   void resetTraversedMap();

   //! debug directions
   void debugDirections(const std::vector<Constants::Direction>& list);

   //! possible directions
   std::vector<Constants::Direction> _directions;

   //! all directions plus current location
   std::vector<Constants::Direction> _directions_and_current;

   //! randomized directions
   std::vector<Constants::Direction> _directions_randomized;

   //! the traversed map
   std::vector<bool> _traversed_positions;

   //! list of reachable positions
   std::vector<Point> _reachable_positions;

   //! list of reachable extra positions
   std::vector<Point> _reachable_extras;
};

#endif  // BOTMAP_H
