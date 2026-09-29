#include "astarpathfinding.h"

#include <cstdio>
#include <format>
#include <limits>
#include <string>

/*!
   \param map astar map to process
*/
void AStarPathFinding::setMap(AStarMap* map)
{
   _node_map = map;
}

/*!
   \param x start x position
   \param y start y position
*/
void AStarPathFinding::setStart(int x, int y)
{
   _start_node = _node_map->getNode(x, y);
}

/*!
   \param x target x position
   \param y target y position
*/
void AStarPathFinding::setTarget(int x, int y)
{
   _target_node = _node_map->getNode(x, y);
}

void AStarPathFinding::findPath()
{
   // init
   int test_g_score = 0;
   bool test_better = false;

   // reset
   _path.clear();
   _open_set.clear();
   _closed_set.clear();

   // add starting node to open list
   if (_start_node)
   {
      _open_set.insert(_start_node);
   }

   while (!_open_set.empty())
   {
      // consider the best node in the open list (the node with the lowest f value)
      // the node in openset having the lowest f_score[] value;
      _current_node = getBestFValueNode(_open_set);

      // this node is the goal
      if (_current_node->getX() == _target_node->getX() && _current_node->getY() == _target_node->getY())
      {
         // then we're done
         _open_set.clear();
         _path = reconstructPath(_current_node);
      }
      else
      {
         // remove current from openset
         _open_set.erase(_current_node);

         // add current to closedset
         _closed_set.insert(_current_node);

         // for (each neighbor) // i.e. up, down, left, right
         for (AStarNode* neighbor : _node_map->getNeighbors(_current_node->getX(), _current_node->getY(), true))
         {
            if (!_closed_set.contains(neighbor))
            {
               test_better = false;
               _current_node->calcG();
               test_g_score = _current_node->getG() + _current_node->getDistance(_target_node);

               if (!_open_set.contains(neighbor))
               {
                  _open_set.insert(neighbor);
                  neighbor->calcH(_target_node);
                  test_better = true;
               }
               else
               {
                  neighbor->calcG();
                  test_better = (test_g_score < neighbor->getG());
               }

               if (test_better)
               {
                  neighbor->setParent(_current_node);
                  neighbor->setG(test_g_score);
                  neighbor->calcF();
               }
            }
         }
      }
   }
}

/*!
   \param current_node node to start from
    eturn all parent nodes
*/
std::vector<AStarNode*> AStarPathFinding::reconstructPath(AStarNode* current_node)
{
   std::vector<AStarNode*> path;

   while (current_node && current_node->getParent())
   {
      path.push_back(current_node);
      current_node = current_node->getParent();
   }

   return path;
}

/*!
   \param set set to scan
    eturn node with best f value
*/
AStarNode* AStarPathFinding::getBestFValueNode(const std::unordered_set<AStarNode*>& set) const
{
   AStarNode* best = nullptr;

   int f_min = std::numeric_limits<int>::max();

   for (AStarNode* candidate : set)
   {
      if (candidate->getF() < f_min)
      {
         best = candidate;
         f_min = best->getF();
      }
   }

   return best;
}

void AStarPathFinding::debugPath()
{
   int i = 0;
   for (AStarNode* node : _path)
   {
      std::printf("%d: (%d, %d)\n", i, node->getX(), node->getY());
      i++;
   }
}

void AStarPathFinding::debugPathShort()
{
   std::string path_string;

   for (AStarNode* node : _path)
   {
      path_string += std::format("=> ({}; {}) ", node->getX(), node->getY());
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
    eturn computed path
*/
const std::vector<AStarNode*>& AStarPathFinding::getPath() const
{
   return _path;
}

/*!
    eturn computed path length
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
