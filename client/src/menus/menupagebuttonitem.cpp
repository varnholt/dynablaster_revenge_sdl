#include "menupagebuttonitem.h"

#include <cmath>

MenuPageButtonItem::MenuPageButtonItem()
{
   _page_item_type = PageItemTypeButton;
   _interactive = true;
}

void MenuPageButtonItem::setFocus(bool focus)
{
   // only allow item focussing when the button is enabled
   if (isEnabled() || !focus)
   {
      if (_focussed && !focus)
      {
         _fade_out = true;
         _focus_out_time.restart();
      }

      MenuPageItem::setFocus(focus);
   }
}

void MenuPageButtonItem::setEnabled(bool enabled)
{
   MenuPageItem::setEnabled(enabled);

   // fade out that button if it's not enabled
   if (!enabled)
   {
      setFocus(false);
   }
}

void MenuPageButtonItem::draw()
{
   MenuPageItem::draw();

   if (isVisible())
   {
      if (_fade_out)
      {
         PSDLayer* psd_layer = getActiveLayer();

         _fade_value = 0.0f;

         if (_focus_out_time.elapsed() < 500)
         {
            _fade_value = std::cos(_focus_out_time.elapsed() * 0.005);
         }

         psd_layer->render(0, 0, _fade_value);

         // either cos drops below 0.0 or focus out time exceeds 300ms
         if (_fade_value < 0.1f)
         {
            _fade_out = false;
         }
      }
   }
}
