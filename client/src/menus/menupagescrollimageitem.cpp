#include "menupagescrollimageitem.h"

#include "clipper.h"
#include "framework/globaltime.h"

#include <cmath>
#include <numbers>

namespace
{
constexpr float SCROLLTIME = 36.0f;
constexpr float LIM_1 = 0.13f * SCROLLTIME;
constexpr float LIM_2 = 0.3333333333f * SCROLLTIME;
constexpr float LIM_3 = 0.6666666666f * SCROLLTIME;
constexpr float LIM_4 = 0.87f * SCROLLTIME;
constexpr float LIM_5 = 1.0f * SCROLLTIME;
constexpr float LIM_2_POS_OFFSET = 0.0f;
constexpr float LIM_2_POS_LENGTH = 0.2f;
constexpr float LIM_3_POS_OFFSET = 0.2f;
constexpr float LIM_3_POS_LENGTH = 0.6f;
constexpr float LIM_4_POS_OFFSET = 0.8f;
constexpr float LIM_4_POS_LENGTH = 0.2f;
}  // namespace

MenuPageScrollImageItem::MenuPageScrollImageItem()
{
   _page_item_type = PageItemTypeScrollImage;
}

MenuPageScrollImageItem::~MenuPageScrollImageItem() = default;

void MenuPageScrollImageItem::initialize()
{
   MenuPageItem::initialize();

   PSDLayer* bounding_rect = getInactiveLayer();

   // use layer for clipping
   _clipper = std::make_unique<Clipper>(
      static_cast<float>(bounding_rect->getLeft()),
      static_cast<float>(bounding_rect->getTop()),
      static_cast<float>(bounding_rect->getRight()),
      static_cast<float>(bounding_rect->getBottom())
   );
}

void MenuPageScrollImageItem::draw()
{
   PSDLayer* layer = getActiveLayer();

   const float offset = _y * (layer->getHeight() - _clipper->getHeight());

   // clip image to reference layer
   _clipper->enable();
   layer->render(0.0f, -offset);  // move up to scroll down
   _clipper->disable();
}

void MenuPageScrollImageItem::reset()
{
   _relative_time_previous = 0.0f;
   _y = 0.0f;
   _move_up = false;
   _start_time = GlobalTime::Instance()->getTime();
}

void MenuPageScrollImageItem::animate(float /*time*/)
{
   _animation_time = GlobalTime::Instance()->getTime();

   const float relative_time = std::fmod(_animation_time - _start_time, LIM_5);

   // flip if fmod limit reached
   if (relative_time < _relative_time_previous)
   {
      _move_up = !_move_up;
   }

   _relative_time_previous = relative_time;

   float position = 0.0f;

   if (relative_time < LIM_1)
   {
      position = 0.0f;
   }
   else if (relative_time < LIM_2)
   {
      // normalize from 0..1, then to pi/2
      float value = (relative_time - LIM_1) / (LIM_2 - LIM_1);
      value *= (std::numbers::pi_v<float> / 2.0f);
      value = 1.0f - std::cos(value);

      position = value * LIM_2_POS_LENGTH + LIM_2_POS_OFFSET;
   }
   else if (relative_time < LIM_3)
   {
      // normalize from 0..1
      const float value = (relative_time - LIM_2) / (LIM_3 - LIM_2);

      position = value * LIM_3_POS_LENGTH + LIM_3_POS_OFFSET;
   }
   else if (relative_time < LIM_4)
   {
      // normalize from 0..1, then to pi/2
      float value = (relative_time - LIM_3) / (LIM_4 - LIM_3);
      value *= (std::numbers::pi_v<float> / 2.0f);
      value = std::sin(value);

      position = value * LIM_4_POS_LENGTH + LIM_4_POS_OFFSET;
   }
   else
   {
      position = 1.0f;
   }

   _y = _move_up ? (1.0f - position) : position;
}
