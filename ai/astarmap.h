#ifndef ASTARMAP_H
#define ASTARMAP_H

// base
#include "botmap.h"
#include "point.h"

#include <cstdint>
#include <vector>

#include "astarnode.h"

class AStarMap : public BotMap
{
public:
   //! constructor
   AStarMap();

   //! constructor
   AStarMap(int width, int height);

   //! create nodes
   void buildNodes();

   //! delete all nodes
   void clearNodes();

   //! get the node indices of all neighbors of a position
   std::vector<int32_t> getNeighbors(int x, int y, bool regard_stones = false) const;

   //! index of the node at x, y
   int32_t getNodeIndex(int x, int y) const;

   //! get node by index
   AStarNode& getNode(int32_t index);
   const AStarNode& getNode(int32_t index) const;

   //! debug path
   void debugPath(const std::vector<Point>& path);

protected:
   //! init map
   void initMap();

   //! check if a point is traversable
   bool isTraversable(const Point& point, bool regard_stones) const;

   //! width * height nodes, indexed by getNodeIndex()
   std::vector<AStarNode> _nodes;
};

#endif  // ASTARMAP_H
