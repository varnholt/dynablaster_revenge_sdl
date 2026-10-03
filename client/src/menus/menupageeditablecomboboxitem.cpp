#include "menupageeditablecomboboxitem.h"

#include "clipper.h"
#include "menupagebuttonitem.h"

MenuPageEditableComboBoxItem::MenuPageEditableComboBoxItem()
{
   _page_item_type = PageItemTypeEditableCombobox;
}

void MenuPageEditableComboBoxItem::updateClipperBounds()
{
   const PSDLayer& layer = _layer_active->get();
   _clipper->setBounds(
      layer.getLeft(), layer.getTop(), layer.getLeft() + layer.getWidth() - 3, layer.getTop() + (layer.getHeight() * getElementCount())
   );
}

void MenuPageEditableComboBoxItem::initialize()
{
   MenuPageComboBoxItem::initialize();
}

void MenuPageEditableComboBoxItem::setTextEditItem(MenuPageTextEditItem& item)
{
   _text_edit_item = item;

   // both items belong to the same MenuPage and share its lifetime
   valueChangedSignal.connect([&item](const std::string& value) { item.setText(value); });
}

std::optional<std::reference_wrapper<MenuPageTextEditItem>> MenuPageEditableComboBoxItem::getTextEditItem() const
{
   return _text_edit_item;
}

void MenuPageEditableComboBoxItem::setVisible(bool visible)
{
   if (visible != isVisible())
   {
      if (const auto button = getButtonItem())
      {
         button->get().setVisible(!visible);
      }

      if (const auto text_edit = getTextEditItem())
      {
         text_edit->get().setVisible(!visible);
      }

      MenuPageListItem::setVisible(visible);
   }
}

void MenuPageEditableComboBoxItem::appendItem(const std::string& item, const Color& color, bool override_alpha, const Color& outline_color)
{
   MenuPageListItem::appendItem(item, color, override_alpha, outline_color);
   updateClipperBounds();
}
