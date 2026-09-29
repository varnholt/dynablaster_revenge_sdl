#include "path.h"


//-----------------------------------------------------------------------------
/*!
*/
Path::Path()
{
}


//-----------------------------------------------------------------------------
/*!
   \param points points to set
*/
void Path::setPoints(const std::vector<Point>& points)
{
   _points = points;
}


//-----------------------------------------------------------------------------
/*!
*/
void Path::positionReached()
{
   _points.erase(_points.begin());
}
