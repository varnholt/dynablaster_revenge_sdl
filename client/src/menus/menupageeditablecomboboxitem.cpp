#include "menupageeditablecomboboxitem.h"

#include "clipper.h"
#include "menupagebuttonitem.h"

std::map<std::string, MenuPageTextEditItem*> MenuPageEditableComboBoxItem::_map_text_edits;

MenuPageEditableComboBoxItem::MenuPageEditableComboBoxItem()
{
   _page_item_type = PageItemTypeEditableCombobox;
}

void MenuPageEditableComboBoxItem::updateClipperBounds()
{
   _clipper->setBounds(
      _layer_active->getLeft(),
      _layer_active->getTop(),
      _layer_active->getLeft() + _layer_active->getWidth() - 3,
      _layer_active->getTop() + (_layer_active->getHeight() * getElementCount())
   );
}

void MenuPageEditableComboBoxItem::initialize()
{
   MenuPageComboBoxItem::initialize();
}

void MenuPageEditableComboBoxItem::setTextEditItem(MenuPageTextEditItem* item)
{
   _text_edit_item = item;
}

MenuPageTextEditItem* MenuPageEditableComboBoxItem::getTextEditItem() const
{
   return _text_edit_item;
}

void MenuPageEditableComboBoxItem::setVisible(bool visible)
{
   if (visible != isVisible())
   {
      if (MenuPageButtonItem* button = getButtonItem())
      {
         button->setVisible(!visible);
      }

      if (MenuPageTextEditItem* text_edit = getTextEditItem())
      {
         text_edit->setVisible(!visible);
      }

      MenuPageListItem::setVisible(visible);
   }
}

void MenuPageEditableComboBoxItem::addTextEdit(const std::string& key, MenuPageTextEditItem* item)
{
   _map_text_edits[key] = item;
}

void MenuPageEditableComboBoxItem::appendItem(const std::string& item, const Color& color, bool override_alpha, const Color& outline_color)
{
   MenuPageListItem::appendItem(item, color, override_alpha, outline_color);
   updateClipperBounds();
}

void MenuPageEditableComboBoxItem::linkComboBoxToTextEdit(const std::string& text_edit_key, const std::string& combo_box_key)
{
   if (_map_text_edits.contains(text_edit_key) && _map_combo_boxes.contains(combo_box_key))
   {
      MenuPageTextEditItem* text_edit = _map_text_edits[text_edit_key];
      auto* editable_combo_box = dynamic_cast<MenuPageEditableComboBoxItem*>(_map_combo_boxes[combo_box_key]);

      editable_combo_box->setTextEditItem(text_edit);

      // both items belong to the same MenuPage and share its lifetime
      editable_combo_box->valueChangedSignal.connect([text_edit](const std::string& value) { text_edit->setText(value); });
   }
}
