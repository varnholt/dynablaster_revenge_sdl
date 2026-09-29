#ifndef ASTARMAP_H
#define ASTARMAP_H

// base
#include "botmap.h"
#include "point.h"

#include <memory>
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

   //! get all neighbor nodes of a position
   std::vector<AStarNode*> getNeighbors(int x, int y, bool regard_stones = false);

   //! get node at x, y
   AStarNode* getNode(int x, int y) const;

   //! debug path
   void debugPath(const std::vector<AStarNode*>& path);

protected:
   //! init map
   void initMap();

   //! check if a point is traversable
   bool isTraversable(const Point& point, bool regard_stones) const;

   //! node map, non-owning lookup into _nodes
   std::vector<AStarNode*> _node_map;

   //! list of nodes
   std::vector<std::unique_ptr<AStarNode>> _nodes;
};

#endif  // ASTARMAP_H
