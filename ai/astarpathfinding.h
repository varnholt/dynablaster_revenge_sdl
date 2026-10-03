#ifndef ASTARPATHFINDING_H
#define ASTARPATHFINDING_H

#include "pathfinding.h"

#include "map.h"
#include "point.h"

#include "astarmap.h"
#include "astarnode.h"

#include <cstdint>
#include <unordered_set>
#include <vector>

class AStarPathFinding : public PathFinding
{
public:
   //! setter for start point
   void setStart(int x, int y);

   //! setter for target point
   void setTarget(int x, int y);

   //! find path from start to target on the given map's nodes
   void findPath(AStarMap& map);

   //! debug path
   void debugPath();

   //! debug path in a short version
   void debugPathShort();

   //! getter for path positions, from the target back to the field after the start
   const std::vector<Point>& getPath() const;

   //! getter for the path length
   int getPathLength() const;

protected:
   //! get index of the best f score node in given set
   int32_t getBestFValueNode(const AStarMap& map, const std::unordered_set<int32_t>& set) const;

   //! number of parent nodes added to the node's g value
   void calcG(const AStarMap& map, AStarNode& node) const;

   //! build path by linking parents
   std::vector<Point> reconstructPath(const AStarMap& map, int32_t current_node) const;

   //! open set of node indices
   std::unordered_set<int32_t> _open_set;

   //! closed set of node indices
   std::unordered_set<int32_t> _closed_set;

   //! start position
   Point _start;

   //! target position
   Point _target;

   //! resulting path
   std::vector<Point> _path;
};

#endif  // ASTARPATHFINDING_H

/*

   a* pathfinding algorithm


   input:
   - start node
   - end node

   item states:
   - walkable
   - non-walkable

   +-----------+
   | g       h |
   |           |
   |     i     |
   |           |
   | f         |
   +-----------+

   g-score: number of parent nodes (length of the path)

   h-score: number of steps between current pos
            and target in steps in vertical or horizontal
            direction (manhattan distance, heuristic value)

   f-score: g-score + h-score

   i: processing index (lower index goes first if tiles have same score)


   Part 1: http://www.youtube.com/watch?v=Kw8AMmyc6vg
   Part 2: http://www.youtube.com/watch?v=uIVu7ViLaZo
   Part 3: http://www.youtube.com/watch?v=kcYkd1Oxnrc


   http://wiki.gamegardens.com/Path_Finding_Tutorial


   http://en.wikipedia.org/wiki/A*_search_algorithm

*/
