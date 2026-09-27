#pragma once

// base
#include "mapitem.h"

#include <memory>

// forward declarations
class ExtraMapItem;

class StoneMapItem : public MapItem
{
   public:

      //! constructor
      StoneMapItem(
         int32_t id,
         int32_t x,
         int32_t y
      );

      //! destructor - out-of-line since mExtraMapItem's unique_ptr needs ExtraMapItem's
      //! complete type, which is only forward-declared in this header
      ~StoneMapItem() override;

      //! setter for extra map item - takes ownership
      void setExtraMapItem(std::unique_ptr<ExtraMapItem>);

      //! getter for extra map item (non-owning observer)
      [[nodiscard]] ExtraMapItem* getExtraMapItem() const;

      //! release ownership of the extra map item (e.g. when it's revealed and becomes its
      //! own independent map item) - returns nullptr if there is none
      [[nodiscard]] std::unique_ptr<ExtraMapItem> releaseExtraMapItem();


   private:

      //! extra map item
      std::unique_ptr<ExtraMapItem> mExtraMapItem;
};
