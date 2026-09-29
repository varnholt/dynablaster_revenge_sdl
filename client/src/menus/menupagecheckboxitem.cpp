#include "menupagecheckboxitem.h"

MenuPageCheckBoxItem::MenuPageCheckBoxItem()
{
   _page_item_type = PageItemTypeCheckbox;
   _interactive = true;
}

void MenuPageCheckBoxItem::setCheckedLayer(PSDLayer* layer)
{
   _layer_checked = layer;
}

void MenuPageCheckBoxItem::setUncheckedLayer(PSDLayer* layer)
{
   _layer_unchecked = layer;
}

PSDLayer* MenuPageCheckBoxItem::getCheckedLayer() const
{
   return _layer_checked;
}

PSDLayer* MenuPageCheckBoxItem::getUncheckedLayer() const
{
   return _layer_unchecked;
}

PSDLayer* MenuPageCheckBoxItem::getLayer() const
{
   return isChecked() ? _layer_checked : _layer_unchecked;
}

bool MenuPageCheckBoxItem::isChecked() const
{
   return _checked;
}

void MenuPageCheckBoxItem::setChecked(bool checked)
{
   _checked = checked;
   stateChangedSignal();
}

void MenuPageCheckBoxItem::toggleChecked()
{
   setChecked(!isChecked());
}

void MenuPageCheckBoxItem::draw()
{
   MenuPageItem::draw();
}

void MenuPageCheckBoxItem::activated()
{
   toggleChecked();
   MenuPageItem::activated();
}

void MenuPageCheckBoxItem::deactivated()
{
   MenuPageItem::deactivated();
}
