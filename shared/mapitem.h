#pragma once

#include <cstdint>
#include <memory>

#include "constants.h"

class MapItemCreatedPacket;

// held by std::shared_ptr wherever it lives on a map (see Map)
class MapItem : public std::enable_shared_from_this<MapItem>
{
public:
   enum ItemType
   {
      Block,
      Bomb,
      Extra,
      Stone,
      Unknown
   };

   MapItem(ItemType type, int32_t appearance, bool blocking, bool destroyable, int32_t x, int32_t y);

   explicit MapItem(const MapItemCreatedPacket& packet);

   virtual ~MapItem() = default;

   [[nodiscard]] ItemType getType() const;

   void setUniqueId(int32_t unique_id);
   [[nodiscard]] int32_t getUniqueId() const;

   [[nodiscard]] bool isBlocking() const;
   [[nodiscard]] bool isDestroyable() const;

   void setX(int32_t x);
   void setY(int32_t y);
   [[nodiscard]] int32_t getX() const;
   [[nodiscard]] int32_t getY() const;

   [[nodiscard]] int32_t getAppearance() const;

   // "being destroyed" flag, avoids double destruction
   void setCurrentlyDestroyed(bool destroyed);
   [[nodiscard]] bool isCurrentlyDestroyed() const;

   // direction from which the item was destroyed
   [[nodiscard]] Constants::Direction getDestroyDirection() const;
   void setDestroyDirection(Constants::Direction direction);

protected:
   void initializeBlocking();

   ItemType _type = Unknown;

   // unique number to identify the item
   int32_t _unique_id = 0;

   // mapping between mesh/texture and id
   int32_t _appearance = 0;

   bool _blocking = false;
   bool _destroyable = false;

   // position on the map
   int32_t _x = 0;
   int32_t _y = 0;

   // continuous counter for unique ids
   static int32_t _current_id;

   bool _currently_destroyed = false;
   Constants::Direction _destroy_direction = Constants::DirectionUnknown;
};
