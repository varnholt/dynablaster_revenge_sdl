#include "astarpathfinding.h"

#include <cstdio>
#include <format>
#include <limits>
#include <string>

/*!
   \param x start x position
   \param y start y position
*/
void AStarPathFinding::setStart(int x, int y)
{
   _start = Point(x, y);
}

/*!
   \param x target x position
   \param y target y position
*/
void AStarPathFinding::setTarget(int x, int y)
{
   _target = Point(x, y);
}

/*!
   \param map astar map to process, its nodes must be built
*/
void AStarPathFinding::findPath(AStarMap& map)
{
   // init
   int test_g_score = 0;
   bool test_better = false;

   // reset
   _path.clear();
   _open_set.clear();
   _closed_set.clear();

   const AStarNode& target_node = map.getNode(map.getNodeIndex(_target.x(), _target.y()));

   // add starting node to open list
   _open_set.insert(map.getNodeIndex(_start.x(), _start.y()));

   while (!_open_set.empty())
   {
      // consider the best node in the open list (the node with the lowest f value)
      // the node in openset having the lowest f_score[] value;
      const int32_t current_index = getBestFValueNode(map, _open_set);
      AStarNode& current_node = map.getNode(current_index);

      // this node is the goal
      if (current_node.getX() == target_node.getX() && current_node.getY() == target_node.getY())
      {
         // then we're done
         _open_set.clear();
         _path = reconstructPath(map, current_index);
      }
      else
      {
         // remove current from openset
         _open_set.erase(current_index);

         // add current to closedset
         _closed_set.insert(current_index);

         // for (each neighbor) // i.e. up, down, left, right
         for (const int32_t neighbor_index : map.getNeighbors(current_node.getX(), current_node.getY(), true))
         {
            if (!_closed_set.contains(neighbor_index))
            {
               AStarNode& neighbor = map.getNode(neighbor_index);

               test_better = false;

               calcG(map, current_node);
               test_g_score = current_node.getG() + current_node.getDistance(target_node);

               if (!_open_set.contains(neighbor_index))
               {
                  _open_set.insert(neighbor_index);
                  neighbor.calcH(target_node);
                  test_better = true;
               }
               else
               {
                  calcG(map, neighbor);
                  test_better = (test_g_score < neighbor.getG());
               }

               if (test_better)
               {
                  neighbor.setParent(current_index);
                  neighbor.setG(test_g_score);
                  neighbor.calcF();
               }
            }
         }
      }
   }
}

/*!
   \param map map holding the nodes
   \param node node to add the number of its parent nodes to
*/
void AStarPathFinding::calcG(const AStarMap& map, AStarNode& node) const
{
   int32_t parent = node.getParent();

   while (parent != -1)
   {
      node.setG(node.getG() + 1);
      parent = map.getNode(parent).getParent();
   }
}

/*!
   \param map map holding the nodes
   \param current_node index of the node to start from
   \return all parent nodes
*/
std::vector<Point> AStarPathFinding::reconstructPath(const AStarMap& map, int32_t current_node) const
{
   std::vector<Point> path;

   while (current_node != -1 && map.getNode(current_node).getParent() != -1)
   {
      const AStarNode& node = map.getNode(current_node);
      path.push_back(Point(node.getX(), node.getY()));
      current_node = node.getParent();
   }

   return path;
}

/*!
   \param map map holding the nodes
   \param set set of node indices to scan
   \return index of the node with the best f value
*/
int32_t AStarPathFinding::getBestFValueNode(const AStarMap& map, const std::unordered_set<int32_t>& set) const
{
   int32_t best = -1;
   int f_min = std::numeric_limits<int>::max();

   for (const int32_t candidate : set)
   {
      if (map.getNode(candidate).getF() < f_min)
      {
         best = candidate;
         f_min = map.getNode(best).getF();
      }
   }

   return best;
}

void AStarPathFinding::debugPath()
{
   int i = 0;

   for (const Point& node : _path)
   {
      std::printf("%d: (%d, %d)\n", i, node.x(), node.y());
      i++;
   }
}

void AStarPathFinding::debugPathShort()
{
   std::string path_string;

   for (const Point& node : _path)
   {
      path_string += std::format("=> ({}; {}) ", node.x(), node.y());
   }

   if (!path_string.empty())
   {
      std::printf("AStarMap::debugPathSimplified: %s\n", path_string.c_str());
   }
   else
   {
      std::printf("AStarMap::debugPathSimplified: path is empty\n");
   }
}

/*!
   \return computed path
*/
const std::vector<Point>& AStarPathFinding::getPath() const
{
   return _path;
}

/*!
   \return computed path length
*/
int AStarPathFinding::getPathLength() const
{
   return static_cast<int>(_path.size());
}

/*
 wiki pseudocode

function A*(start,goal)
     closedset := the empty set    // The set of nodes already evaluated.
     openset := {start}    // The set of tentative nodes to be evaluated, initially containing the start node
     came_from := the empty map    // The map of navigated nodes.

     g_score[start] := 0    // Cost from start along best known path.
     h_score[start] := heuristic_cost_estimate(start, goal)
     f_score[start] := g_score[start] + h_score[start]    // Estimated total cost from start to goal through y.

     while openset is not empty
         current := the node in openset having the lowest f_score[] value
         if current = goal
             return reconstruct_path(came_from, came_from[goal])

         remove current from openset
         add current to closedset

         for each neighbor in neighbor_nodes(current)
         {
             if neighbor in closedset
                 continue

             tentative_g_score := g_score[current] + dist_between(current,neighbor)

             if neighbor not in openset
                 add neighbor to openset
                 h_score[neighbor] := heuristic_cost_estimate(neighbor, goal)
                 tentative_is_better := true
             else if tentative_g_score < g_score[neighbor]
                 tentative_is_better := true
             else
                 tentative_is_better := false

             if tentative_is_better = true
                 came_from[neighbor] := current
                 g_score[neighbor] := tentative_g_score
                 f_score[neighbor] := g_score[neighbor] + h_score[neighbor]
         }

     return failure

 function reconstruct_path(came_from, current_node)
     if came_from[current_node] is set
         p := reconstruct_path(came_from, came_from[current_node])
         return (p + current_node)
     else
         return current_node
*/
