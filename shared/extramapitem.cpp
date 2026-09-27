
// header
#include "extramapitem.h"

// shared
#include "extramapitemcreatedpacket.h"

//-----------------------------------------------------------------------------
/*!
  constructor
*/
ExtraMapItem::ExtraMapItem(
   int32_t id,
   Constants::ExtraType type,
   int32_t x,
   int32_t y
)
   : MapItem(Extra, id, false, true, x, y),
     mExtraType(type)
{
}


//-----------------------------------------------------------------------------
/*!
   \param packet packet to construct extramapitem from
*/
ExtraMapItem::ExtraMapItem(ExtraMapItemCreatedPacket* packet)
 : MapItem(packet),
   mExtraType(static_cast<Constants::ExtraType>(packet->getExtraType()))
{
}



//-----------------------------------------------------------------------------
/*!
*/
ExtraMapItem::~ExtraMapItem()
{
}


//-----------------------------------------------------------------------------
/*!
   \return the extra's type
*/
Constants::ExtraType ExtraMapItem::getExtraType() const
{
   return mExtraType;
}



//-----------------------------------------------------------------------------
/*!
*/
void ExtraMapItem::initializeStartTime()
{
   mStartTime.start();
}



//-----------------------------------------------------------------------------
/*!
   \return elapsed time
*/
float ExtraMapItem::getElapsedTime() const
{
   return mStartTime.elapsed() * 0.001f;
}


//-----------------------------------------------------------------------------
/*!
   \param faces skull faces
*/
void ExtraMapItem::setSkullFaces(const std::vector<Constants::SkullType>& faces)
{
   mSkullFaces = faces;
}


//-----------------------------------------------------------------------------
/*!
   \return skull faces
*/
std::vector<Constants::SkullType> ExtraMapItem::getSkullFaces() const
{
   return mSkullFaces;
}


