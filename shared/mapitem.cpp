#include "mapitem.h"

#include "mapitemcreatedpacket.h"

int32_t MapItem::_current_id = 0;

MapItem::MapItem(ItemType type, int32_t appearance, bool blocking, bool destroyable, int32_t x, int32_t y)
    : _type(type), _unique_id(_current_id++), _appearance(appearance), _blocking(blocking), _destroyable(destroyable), _x(x), _y(y)
{
   initializeBlocking();
}

MapItem::MapItem(const MapItemCreatedPacket& packet)
    : _type(packet.getItemType()),
      _unique_id(packet.getUniqueId()),
      _appearance(packet.getAppearance()),
      _x(packet.getX()),
      _y(packet.getY())
{
   initializeBlocking();
}

void MapItem::initializeBlocking()
{
   switch (_type)
   {
      case Block:
         _blocking = true;
         break;

      case Bomb:
         _blocking = true;
         break;

      case Stone:
         _blocking = true;
         _destroyable = true;
         break;

      // extras are non-blocking
      default:
         break;
   }
}

MapItem::ItemType MapItem::getType() const
{
   return _type;
}

void MapItem::setUniqueId(int32_t unique_id)
{
   _unique_id = unique_id;
}

int32_t MapItem::getAppearance() const
{
   return _appearance;
}

bool MapItem::isBlocking() const
{
   return _blocking;
}

bool MapItem::isDestroyable() const
{
   return _destroyable;
}

void MapItem::setX(int32_t x)
{
   _x = x;
}

void MapItem::setY(int32_t y)
{
   _y = y;
}

int32_t MapItem::getX() const
{
   return _x;
}

int32_t MapItem::getY() const
{
   return _y;
}

int32_t MapItem::getUniqueId() const
{
   return _unique_id;
}

void MapItem::setCurrentlyDestroyed(bool destroyed)
{
   _currently_destroyed = destroyed;
}

bool MapItem::isCurrentlyDestroyed() const
{
   return _currently_destroyed;
}

Constants::Direction MapItem::getDestroyDirection() const
{
   return _destroy_direction;
}

void MapItem::setDestroyDirection(Constants::Direction direction)
{
   _destroy_direction = direction;
}
