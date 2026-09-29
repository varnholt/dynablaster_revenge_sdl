// header
#include "astarnode.h"

// math
#include <stdlib.h>


//-----------------------------------------------------------------------------
/*!
   \param parent parent node
*/
AStarNode::AStarNode(AStarNode* parent)
   : _parent(parent),
     _x(0),
     _y(0),
     _g(0),
     _h(0),
     _f(0)
{
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
*/
void AStarNode::setX(int x)
{
   _x = x;
}


//-----------------------------------------------------------------------------
/*!
   \param y y position
*/
void AStarNode::setY(int y)
{
   _y = y;
}


//-----------------------------------------------------------------------------
/*!
   \param parent parent node
*/
void AStarNode::setParent(AStarNode* parent)
{
   _parent = parent;
}


//-----------------------------------------------------------------------------
/*!
   \return parent node
*/
AStarNode* AStarNode::getParent() const
{
   return _parent;
}

//-----------------------------------------------------------------------------
/*!
   \return x position
*/
int AStarNode::getX() const
{
   return _x;
}


//-----------------------------------------------------------------------------
/*!
   \return y position
*/
int AStarNode::getY() const
{
   return _y;
}


//-----------------------------------------------------------------------------
/*!
   \param g g value
*/
void AStarNode::setG(int g)
{
   _g = g;
}


//-----------------------------------------------------------------------------
/*!
   \return g value
*/
int AStarNode::getG() const
{
   return _g;
}


//-----------------------------------------------------------------------------
/*!
   \return h value
*/
int AStarNode::getH() const
{
   return _h;
}


//-----------------------------------------------------------------------------
/*!
   \return f value
*/
int AStarNode::getF() const
{
   return _f;
}


//-----------------------------------------------------------------------------
/*!
*/
void AStarNode::calcG()
{
   AStarNode* parent = _parent;

   while (parent)
   {
      _g++;
      parent = parent->_parent;
   }
}


//-----------------------------------------------------------------------------
/*!
   \param target target node
   \return distance to given node
*/
int AStarNode::getDistance(AStarNode *target)
{
   return abs(_x - target->getX()) + abs(_y + target->getY());;
}


//-----------------------------------------------------------------------------
/*!
   \param target target to calculate h to
*/
void AStarNode::calcH(AStarNode* target)
{
   _h = getDistance(target);
}


//-----------------------------------------------------------------------------
/*!
*/
void AStarNode::calcF()
{
   _f = _g + _h;
}

