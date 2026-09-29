#include "astarmap.h"

// shared
#include "mapitem.h"

#include <algorithm>
#include <cstdio>
#include <format>
#include <string>
#include <vector>

AStarMap::AStarMap()
{
   initMap();
}

AStarMap::AStarMap(int width, int height) : BotMap(width, height)
{
   initMap();
}

void AStarMap::initMap()
{
   _node_map.assign(static_cast<size_t>(getWidth() * getHeight()), nullptr);
}

void AStarMap::buildNodes()
{
   const int width = getWidth();

   for (int x = 0; x < width; x++)
   {
      for (int y = 0; y < getHeight(); y++)
      {
         // init node
         auto node = std::make_unique<AStarNode>();
         node->setX(x);
         node->setY(y);

         // add node to map
         _node_map[y * width + x] = node.get();

         // add node to node list
         _nodes.push_back(std::move(node));
      }
   }
}

void AStarMap::clearNodes()
{
   std::ranges::fill(_node_map, nullptr);
   _nodes.clear();
}

/*!
  \param x x position
  \param y y position
  \param regard_stones \c true if stones are to be regarded
  \return list of neighbors
*/
std::vector<AStarNode*> AStarMap::getNeighbors(int x, int y, bool regard_stones)
{
   std::vector<AStarNode*> list;

   const Point up(x, y - 1);
   const Point down(x, y + 1);
   const Point left(x - 1, y);
   const Point right(x + 1, y);

   if (up.y() >= 0 && isTraversable(up, regard_stones))
   {
      list.push_back(getNode(up.x(), up.y()));
   }

   if (down.y() < getHeight() && isTraversable(down, regard_stones))
   {
      list.push_back(getNode(down.x(), down.y()));
   }

   if (left.x() >= 0 && isTraversable(left, regard_stones))
   {
      list.push_back(getNode(left.x(), left.y()));
   }

   if (right.x() < getWidth() && isTraversable(right, regard_stones))
   {
      list.push_back(getNode(right.x(), right.y()));
   }

   return list;
}

/*!
  \param point point to check
  \param regard_stones \c if stones are to be regarded
  \return true if point is traversable
*/
bool AStarMap::isTraversable(const Point& point, bool regard_stones) const
{
   MapItem* item = getItem(point.x(), point.y());
   bool add = true;

   if (item && item->getType() == MapItem::Bomb)
   {
      add = false;
   }

   if (item && item->getType() == MapItem::Block)
   {
      add = false;
   }

   if (add && regard_stones && item && item->getType() == MapItem::Stone)
   {
      add = false;
   }

   return add;
}

/*!
   \return pointer to node
   \param x x position
   \param y y position
*/
AStarNode* AStarMap::getNode(int x, int y) const
{
   return _node_map[y * getWidth() + x];
}

/*!
  \param path path to debug
*/
void AStarMap::debugPath(const std::vector<AStarNode*>& path)
{
   const int width = getWidth();
   std::vector<AStarNode*> map(static_cast<size_t>(width * getHeight()), nullptr);

   for (AStarNode* node : path)
   {
      map[node->getY() * width + node->getX()] = node;
   }

   std::string joined;

   for (int yi = 0; yi < getHeight(); yi++)
   {
      std::string line = std::format("{:x}| ", yi);

      for (int xi = 0; xi < width; xi++)
      {
         char c = ' ';

         if (map[yi * width + xi])
         {
            c = 'x';
         }
         else if (MapItem* item = getItem(xi, yi))
         {
            switch (item->getType())
            {
               case MapItem::Block:
                  c = '#';
                  break;
               case MapItem::Bomb:
                  c = 'B';
                  break;
               case MapItem::Extra:
                  c = 'E';
                  break;
               case MapItem::Stone:
                  c = 'S';
                  break;
               case MapItem::Unknown:
               default:
                  c = ' ';
                  break;
            }
         }

         line.push_back(c);
      }

      if (!joined.empty())
      {
         joined += '\n';
      }

      joined += line;
   }

   std::printf(" | 012345678901234567890123456789\n");
   std::printf(" +---------------\n");
   std::printf("%s\n", joined.c_str());
   std::printf(" +---------------\n");
   std::printf(" | 012345678901234567890123456789\n");
}
