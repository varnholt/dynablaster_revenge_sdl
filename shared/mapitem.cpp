// header
#include "mapitem.h"

// shared
#include "mapitemcreatedpacket.h"


//! mapitem id
int32_t MapItem::mCurrentId = 0;


//-----------------------------------------------------------------------------
/*!
   \param type mapitem type
   \param appearance mapitem appearance
   \param blocking blocking flag
   \param destroyable destroyable flag
   \param x x position
   \param y y position
*/
MapItem::MapItem(
   ItemType type,
   int32_t appearance,
   bool blocking,
   bool destroyable,
   int32_t x,
   int32_t y
)
 : mType(type),
   mUniqueId(mCurrentId++),
   mAppearance(appearance),
   mBlocking(blocking),
   mDestroyable(destroyable),
   mX(x),
   mY(y),
   mCurrentlyDestroyed(false),
   mDestroyDirection(Constants::DirectionUnknown)
{
   initializeBlocking();
}


//-----------------------------------------------------------------------------
/*!
   \param packet packet to construct mapitem from
*/
MapItem::MapItem(MapItemCreatedPacket *pack)
 : mType(pack->getItemType()),
   mUniqueId(pack->getUniqueId()),
   mAppearance(pack->getAppearance()),
   mBlocking(false),
   mDestroyable(false),
   mX(pack->getX()),
   mY(pack->getY()),
   mCurrentlyDestroyed(false),
   mDestroyDirection(Constants::DirectionUnknown)
{
   initializeBlocking();
}


//-----------------------------------------------------------------------------
/*!
*/
MapItem::~MapItem()
{
}


//-----------------------------------------------------------------------------
/*!
*/
void MapItem::initializeBlocking()
{
   switch (mType)
   {
      case Block:
         mBlocking = true;
         break;

      case Bomb:
         mBlocking = true;
         break;

      case Stone:
         mBlocking = true;
         mDestroyable = true;
         break;

      // extras are non-blocking
      default:
         break;
   }
}


//-----------------------------------------------------------------------------
/*!
   \return item type
*/
MapItem::ItemType MapItem::getType() const
{
   return mType;
}


//-----------------------------------------------------------------------------
/*!
   \param uniqueId item's unique id
*/
void MapItem::setUniqueId(int32_t uniqueId)
{
   mUniqueId = uniqueId;
}


//-----------------------------------------------------------------------------
/*!
   \return item appearance
*/
int32_t MapItem::getAppearance() const
{
   return mAppearance;
}


//-----------------------------------------------------------------------------
/*!
   \return true if item is blocking
*/
bool MapItem::isBlocking() const
{
   return mBlocking;
}


//-----------------------------------------------------------------------------
/*!
   \return true if item is destroyable
*/
bool MapItem::isDestroyable() const
{
   return mDestroyable;
}


//-----------------------------------------------------------------------------
/*!
   \param x x position
*/
void MapItem::setX(int32_t x)
{
   mX = x;
}


//-----------------------------------------------------------------------------
/*!
   \param y y position
*/
void MapItem::setY(int32_t y)
{
   mY = y;
}


//-----------------------------------------------------------------------------
/*!
   \return x position
*/
int32_t MapItem::getX() const
{
   return mX;
}


//-----------------------------------------------------------------------------
/*!
   \return y position
*/
int32_t MapItem::getY() const
{
   return mY;
}


//-----------------------------------------------------------------------------
/*!
   \return unique mapitem id
*/
int32_t MapItem::getUniqueId() const
{
   return mUniqueId;
}


//-----------------------------------------------------------------------------
/*!
  \param destroyed true if item is currently being destroyed
*/
void MapItem::setCurrentlyDestroyed(bool destroyed)
{
   mCurrentlyDestroyed = destroyed;
}


//-----------------------------------------------------------------------------
/*!
   \return true if mapitem is currently being destroyed
*/
bool MapItem::isCurrentlyDestroyed() const
{
   return mCurrentlyDestroyed;
}


//-----------------------------------------------------------------------------
/*!
   \return direction from which the item was destroyed
*/
Constants::Direction MapItem::getDestroyDirection() const
{
   return mDestroyDirection;
}


//-----------------------------------------------------------------------------
/*!
   \param direction direction from which the item was destroyed
*/
void MapItem::setDestroyDirection(Constants::Direction direction)
{
   mDestroyDirection= direction;
}

