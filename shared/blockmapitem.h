#pragma once

// base
#include "mapitem.h"

class BlockMapItem : public MapItem
{
   public:

      //! constructor
      BlockMapItem(
         int32_t id,
         int32_t x,
         int32_t y
      );
};
