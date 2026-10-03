#include "menupageslideritem.h"

MenuPageSliderItem::MenuPageSliderItem()
{
   _page_item_type = PageItemTypeSlider;
   _interactive = true;
}

void MenuPageSliderItem::mousePressed(int x, int y)
{
   _relative_to_x = x - _layer_active->get().getLeft();

   setActive(true);
   MenuPageItem::mousePressed(x, y);
}

void MenuPageSliderItem::mouseReleased()
{
   setActive(false);
   MenuPageItem::mouseReleased();
}

void MenuPageSliderItem::mouseMoved(int x, int y)
{
   x -= _relative_to_x;

   float width = _maximum - _minimum;

   if (x > _maximum - _layer_active->get().getWidth())
   {
      x = _maximum - _layer_active->get().getWidth();
   }
   else if (x < _minimum)
   {
      x = _minimum;
   }

   _layer_active->get().getLayer().setX(x);

   _value = (x - _minimum) / width;
   valueChangedSignal(_value);

   MenuPageItem::mouseMoved(x, y);
}

bool MenuPageSliderItem::isGrabbingMouseEvents()
{
   return true;
}

void MenuPageSliderItem::setMinimum(int minimum)
{
   _minimum = minimum;
}

void MenuPageSliderItem::setMaximum(int maximum)
{
   _maximum = maximum;
}

int MenuPageSliderItem::getMinimum() const
{
   return _minimum;
}

int MenuPageSliderItem::getMaximum() const
{
   return _maximum;
}

float MenuPageSliderItem::getValue() const
{
   return _value;
}

void MenuPageSliderItem::setValue(float value)
{
   // reset mouse press relative x
   _relative_to_x = 0;

   _value = value;

   // determine new x position
   float width = _maximum - _minimum;

   float x = _minimum + width * _value;

   // the slider knob takes a few pixels, too => get rid of half of its width
   x -= _layer_active->get().getLayer().getWidth() * 0.5f;
   if (x < _minimum)
   {
      x = _minimum;
   }

   _layer_active->get().getLayer().setX(x);
}
