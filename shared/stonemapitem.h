#pragma once

#include "mapitem.h"

#include <memory>

class ExtraMapItem;

class StoneMapItem : public MapItem
{
public:
   StoneMapItem(int32_t id, int32_t x, int32_t y);

   // out-of-line since the unique_ptr needs ExtraMapItem's complete type
   ~StoneMapItem() override;

   // takes ownership
   void setExtraMapItem(std::unique_ptr<ExtraMapItem> item);

   [[nodiscard]] bool hasExtraMapItem() const;

   // releases ownership (e.g. when the extra is revealed and becomes its own map item), nullptr if none
   [[nodiscard]] std::unique_ptr<ExtraMapItem> releaseExtraMapItem();

private:
   std::unique_ptr<ExtraMapItem> _extra_map_item;
};
