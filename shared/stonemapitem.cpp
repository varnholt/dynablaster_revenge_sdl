#include "stonemapitem.h"

#include "extramapitem.h"

StoneMapItem::StoneMapItem(int32_t id, int32_t x, int32_t y) : MapItem(Stone, id, true, true, x, y)
{
}

StoneMapItem::~StoneMapItem() = default;

void StoneMapItem::setExtraMapItem(std::unique_ptr<ExtraMapItem> item)
{
   _extra_map_item = std::move(item);
}

bool StoneMapItem::hasExtraMapItem() const
{
   return _extra_map_item != nullptr;
}

std::unique_ptr<ExtraMapItem> StoneMapItem::releaseExtraMapItem()
{
   return std::move(_extra_map_item);
}
