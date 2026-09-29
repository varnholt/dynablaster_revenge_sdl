#ifndef ASTARNODE_H
#define ASTARNODE_H

class AStarNode
{
   public:

      //! constructor
      AStarNode(AStarNode* parent = 0);

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
      AStarNode* _parent;

      //! x position
      int _x;

      //! y position
      int _y;

      //! g value
      int _g;

      //! h value
      int _h;

      //! f value
      int _f;

};

#endif // ASTARNODE_H


