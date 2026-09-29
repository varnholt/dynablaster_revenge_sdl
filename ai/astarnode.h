#ifndef ASTARNODE_H
#define ASTARNODE_H

class AStarNode
{
public:
   //! constructor
   explicit AStarNode(AStarNode* parent = nullptr);

   //! setter for x
   void setX(int x);

   //! setter for y
   void setY(int y);

   //! getter for x
   int getX() const;

   //! getter for y
   int getY() const;

   //! setter for parent node
   void setParent(AStarNode* parent);

   //! getter for parent node
   AStarNode* getParent() const;

   //! setter for g function value
   void setG(int g);

   //! getter for g function value
   int getG() const;

   //! getter for h function value
   int getH() const;

   //! getter for f function value
   int getF() const;

   //! calculate number of parent nodes
   void calcG();

   //! calculate heuristic value
   void calcH(AStarNode* target);

   //! calculate sum
   void calcF();

   //! getter for distance from this to target node
   int getDistance(AStarNode* target);

protected:
   //! parent node
   AStarNode* _parent = nullptr;

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
