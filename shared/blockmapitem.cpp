#include "blockmapitem.h"

BlockMapItem::BlockMapItem(
   int32_t id,
   int32_t x,
   int32_t y
)
   : MapItem(Block, id, true, false, x, y)
{
}
