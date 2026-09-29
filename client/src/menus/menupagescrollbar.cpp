#include "menupagescrollbar.h"

MenuPageScrollbar::MenuPageScrollbar()
{
   _page_item_type = PageItemTypeScrollbar;
   _interactive = true;
}

void MenuPageScrollbar::mousePressed(int x, int y)
{
   _relative_to_y = y - _layer_active->getTop();

   setActive(true);
   MenuPageItem::mousePressed(x, y);
}

void MenuPageScrollbar::mouseReleased()
{
   setActive(false);
   MenuPageItem::mouseReleased();
}

void MenuPageScrollbar::mouseMoved(int x, int y)
{
   y -= _relative_to_y;

   if (y + _layer_active->getHeight() > _top + _height)
   {
      _position = _top + _height - _layer_active->getHeight();
   }
   else if (y < _top)
   {
      _position = _top;
   }
   else
   {
      _position = y;
   }

   _layer_active->getLayer()->setY(_position);

   _offset = (_position - _top) / static_cast<float>(_height - _layer_active->getHeight());

   if (!_signals_blocked)
   {
      scrollToPercentageSignal(_offset);
   }

   MenuPageItem::mouseMoved(x, y);
}

bool MenuPageScrollbar::isGrabbingMouseEvents()
{
   return true;
}

void MenuPageScrollbar::setHeight(int height)
{
   _height = height;
}

void MenuPageScrollbar::setTop(int top)
{
   _top = top;
}

void MenuPageScrollbar::updateFromAnimation(float percent)
{
   // reset mouse press relative y
   _relative_to_y = 0;

   _signals_blocked = true;
   mouseMoved(0, _top + ((_height - _layer_active->getHeight()) * percent));
   _signals_blocked = false;
}
