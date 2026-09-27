#pragma once

#include <cstdint>

#include "constants.h"

// forward declarations
class MapItemCreatedPacket;

class MapItem
{
   public:

      //! item types
      enum ItemType
      {
         Block,
         Bomb,
         Extra,
         Stone,
         Unknown
      };

      //! construct from data
      MapItem(
         ItemType type,
         int32_t id,
         bool blocking,
         bool destroyable,
         int32_t posX,
         int32_t posY
      );

      //! construct from packet
      MapItem(MapItemCreatedPacket *packet);

      //! destructor
      virtual ~MapItem();

      //! getter for item type
      [[nodiscard]] ItemType getType() const;

      //! setter for unique id
      void setUniqueId(int32_t);

      //! getter for unique id
      [[nodiscard]] int32_t getUniqueId() const;

      //! getter for blocking flag
      [[nodiscard]] bool isBlocking() const;

      //! getter for destroyable flag
      [[nodiscard]] bool isDestroyable() const;

      //! setter for x position
      void setX(int32_t x);

      //! setter for y position
      void setY(int32_t y);

      //! getter for x position
      [[nodiscard]] int32_t getX() const;

      //! getter for y position
      [[nodiscard]] int32_t getY() const;

      //! getter for appearance
      [[nodiscard]] int32_t getAppearance() const;

      //! setter for "being destroyed" flag
      void setCurrentlyDestroyed(bool destroyed);

      //! getter for "being destroyed" flag
      [[nodiscard]] bool isCurrentlyDestroyed() const;

      //! get direction from which the item was destroyed
      [[nodiscard]] Constants::Direction getDestroyDirection() const;

      //! set direction from which the item was destroyed
      void setDestroyDirection(Constants::Direction direction);


   protected:

      //! initialize blocking flag
      void initializeBlocking();

      //! item type
      ItemType mType;

      //! unique number to identify the item
      int32_t mUniqueId;

      //! item id (mapping between mesh/texture and id)
      int32_t mAppearance;

      //! item is blocking
      bool mBlocking;

      //! item is destroyable
      bool mDestroyable;

      //! item's x position on the map
      int32_t mX;

      //! item's y position on the map
      int32_t mY;

      //! continuous counter for unique id
      static int32_t mCurrentId;

      //! mapitem is currently being destroyed (avoid "double-destruction")
      bool mCurrentlyDestroyed;

      //! direction from which the item was destroyed
      Constants::Direction mDestroyDirection;
};
