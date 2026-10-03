#ifndef ASTARNODE_H
#define ASTARNODE_H

#include <cstdint>

class AStarNode
{
public:
   //! setter for x
   void setX(int x);

   //! setter for y
   void setY(int y);

   //! getter for x
   int getX() const;

   //! getter for y
   int getY() const;

   //! setter for the parent node's index in the AStarMap, -1 for none
   void setParent(int32_t parent);

   //! getter for the parent node's index in the AStarMap, -1 for none
   int32_t getParent() const;

   //! setter for g function value
   void setG(int g);

   //! getter for g function value
   int getG() const;

   //! getter for h function value
   int getH() const;

   //! getter for f function value
   int getF() const;

   //! calculate heuristic value
   void calcH(const AStarNode& target);

   //! calculate sum
   void calcF();

   //! getter for distance from this to target node
   int getDistance(const AStarNode& target) const;

protected:
   //! parent node index
   int32_t _parent = -1;

   //! x position
   int _x = 0;

   //! y position
   int _y = 0;

   //! g value
   int _g = 0;

   //! h value
   int _h = 0;

   //! f value
   int _f = 0;
};

#endif  // ASTARNODE_H
