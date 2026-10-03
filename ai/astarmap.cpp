#include "astarmap.h"

// shared
#include "mapitem.h"

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
   _nodes.clear();
}

void AStarMap::buildNodes()
{
   const int width = getWidth();

   // fresh nodes for every search
   _nodes.assign(static_cast<size_t>(width * getHeight()), AStarNode{});

   for (int x = 0; x < width; x++)
   {
      for (int y = 0; y < getHeight(); y++)
      {
         AStarNode& node = _nodes[static_cast<size_t>(getNodeIndex(x, y))];
         node.setX(x);
         node.setY(y);
      }
   }
}

void AStarMap::clearNodes()
{
   _nodes.clear();
}

/*!
  \param x x position
  \param y y position
  \param regard_stones \c true if stones are to be regarded
  \return list of neighbor node indices
*/
std::vector<int32_t> AStarMap::getNeighbors(int x, int y, bool regard_stones) const
{
   std::vector<int32_t> list;

   const Point up(x, y - 1);
   const Point down(x, y + 1);
   const Point left(x - 1, y);
   const Point right(x + 1, y);

   if (up.y() >= 0 && isTraversable(up, regard_stones))
   {
      list.push_back(getNodeIndex(up.x(), up.y()));
   }

   if (down.y() < getHeight() && isTraversable(down, regard_stones))
   {
      list.push_back(getNodeIndex(down.x(), down.y()));
   }

   if (left.x() >= 0 && isTraversable(left, regard_stones))
   {
      list.push_back(getNodeIndex(left.x(), left.y()));
   }

   if (right.x() < getWidth() && isTraversable(right, regard_stones))
   {
      list.push_back(getNodeIndex(right.x(), right.y()));
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
   const auto& item = getItem(point.x(), point.y());

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
   \param x x position
   \param y y position
   \return index of the node at x, y
*/
int32_t AStarMap::getNodeIndex(int x, int y) const
{
   return y * getWidth() + x;
}

/*!
   \param index node index
   \return node
*/
AStarNode& AStarMap::getNode(int32_t index)
{
   return _nodes[static_cast<size_t>(index)];
}

/*!
   \param index node index
   \return node
*/
const AStarNode& AStarMap::getNode(int32_t index) const
{
   return _nodes[static_cast<size_t>(index)];
}

/*!
  \param path path to debug
*/
void AStarMap::debugPath(const std::vector<Point>& path)
{
   const int width = getWidth();
   std::vector<bool> map(static_cast<size_t>(width * getHeight()), false);

   for (const Point& node : path)
   {
      map[static_cast<size_t>(node.y() * width + node.x())] = true;
   }

   std::string joined;

   for (int yi = 0; yi < getHeight(); yi++)
   {
      std::string line = std::format("{:x}| ", yi);

      for (int xi = 0; xi < width; xi++)
      {
         char c = ' ';

         if (map[static_cast<size_t>(yi * width + xi)])
         {
            c = 'x';
         }
         else if (const auto& item = getItem(xi, yi))
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
