#include "menupageitem.h"

MenuPageItem::PageItemType MenuPageItem::getPageItemType() const
{
   return _page_item_type;
}

void MenuPageItem::initialize()
{
}

void MenuPageItem::setActiveLayer(PSDLayer* layer)
{
   _layer_active = layer;
}

void MenuPageItem::setInactiveLayer(PSDLayer* layer)
{
   _layer_inactive = layer;
}

void MenuPageItem::setFocus(bool focus)
{
   _focussed = focus;
}

bool MenuPageItem::isFocussed() const
{
   return _focussed;
}

bool MenuPageItem::isModal() const
{
   return false;
}

void MenuPageItem::activated()
{
   actionSignal(_action);
}

void MenuPageItem::deactivated()
{
}

void MenuPageItem::setInteractive(bool interactive)
{
   _interactive = interactive;
}

bool MenuPageItem::isInteractive()
{
   return _interactive;
}

void MenuPageItem::setAction(const std::string& action)
{
   _action = action;
}

bool MenuPageItem::isActive()
{
   return _active;
}

void MenuPageItem::setActive(bool active)
{
   _active = active;
}

void MenuPageItem::draw()
{
   if (isVisible())
   {
      if (PSDLayer* psd_layer = getLayer())
      {
         psd_layer->render();
      }
   }
}

PSDLayer* MenuPageItem::getLayer() const
{
   return isFocussed() ? _layer_active : _layer_inactive;
}

PSD::Layer* MenuPageItem::getCurrentLayer()
{
   PSDLayer* layer = getLayer();
   return layer ? layer->getLayer() : nullptr;
}

PSDLayer* MenuPageItem::getActiveLayer()
{
   return _layer_active;
}

PSDLayer* MenuPageItem::getInactiveLayer()
{
   return _layer_inactive;
}

void MenuPageItem::keyPressed(int /*key*/, const std::string& /*text*/)
{
}

void MenuPageItem::setEnabled(bool enabled)
{
   _enabled = enabled;
}

MenuPageItem* MenuPageItem::getParent() const
{
   return _parent;
}

void MenuPageItem::setParent(MenuPageItem* value)
{
   _parent = value;
}

bool MenuPageItem::hasNestedElements()
{
   return false;
}

bool MenuPageItem::isGrabbingMouseEvents()
{
   return false;
}

void MenuPageItem::mouseMoved(int /*x*/, int /*y*/)
{
}

void MenuPageItem::mousePressed(int /*x*/, int /*y*/)
{
}

void MenuPageItem::mouseReleased()
{
   mouseReleasedSignal();
}

void MenuPageItem::setVisible(bool visible)
{
   _visible = visible;
   visibleSignal(visible);
}

bool MenuPageItem::isVisible() const
{
   return _visible;
}

bool MenuPageItem::isEnabled() const
{
   return _enabled;
}

void MenuPageItem::setTabIndex(int tab_index)
{
   _tab_index = tab_index;
}

int MenuPageItem::getTabIndex() const
{
   return _tab_index;
}

bool MenuPageItem::isActionRequestOnClickEnabled() const
{
   return true;
}
