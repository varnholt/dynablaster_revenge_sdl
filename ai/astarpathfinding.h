#ifndef ASTARPATHFINDING_H
#define ASTARPATHFINDING_H

#include "pathfinding.h"

#include "map.h"

#include "astarmap.h"
#include "astarnode.h"

#include <unordered_set>
#include <vector>

class AStarPathFinding : public PathFinding
{
public:
   //! setter for map
   void setMap(AStarMap* map);

   //! setter for start point
   void setStart(int x, int y);

   //! setter for target point
   void setTarget(int x, int y);

   //! find path from start to target
   void findPath();

   //! debug path
   void debugPath();

   //! debug path in a short version
   void debugPathShort();

   //! getter for path nodes
   const std::vector<AStarNode*>& getPath() const;

   //! getter for the path length
   int getPathLength() const;

protected:
   //! get best f score node in given set
   AStarNode* getBestFValueNode(const std::unordered_set<AStarNode*>& set) const;

   //! build path by linking parents
   std::vector<AStarNode*> reconstructPath(AStarNode* current_node);

   //! open set
   std::unordered_set<AStarNode*> _open_set;

   //! closed set
   std::unordered_set<AStarNode*> _closed_set;

   //! start node
   AStarNode* _start_node = nullptr;

   //! target node
   AStarNode* _target_node = nullptr;

   //! current node
   AStarNode* _current_node = nullptr;

   //! map to work on
   AStarMap* _node_map = nullptr;

   //! resulting path
   std::vector<AStarNode*> _path;
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
