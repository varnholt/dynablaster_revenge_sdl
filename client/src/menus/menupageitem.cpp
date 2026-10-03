#include "menupageitem.h"

MenuPageItem::PageItemType MenuPageItem::getPageItemType() const
{
   return _page_item_type;
}

void MenuPageItem::initialize()
{
}

void MenuPageItem::setActiveLayer(PSDLayer& layer)
{
   _layer_active = layer;
}

void MenuPageItem::setInactiveLayer(PSDLayer& layer)
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

bool MenuPageItem::isInteractive()
{
   return _interactive;
}

void MenuPageItem::setInteractive(bool interactive)
{
   _interactive = interactive;
}

void MenuPageItem::setLayerDrawn(bool drawn)
{
   _layer_drawn = drawn;
}

bool MenuPageItem::isLayerDrawn() const
{
   return _layer_drawn;
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
   if (isVisible() && isLayerDrawn())
   {
      if (const auto psd_layer = getLayer())
      {
         psd_layer->get().render();
      }
   }
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageItem::getLayer() const
{
   return isFocussed() ? _layer_active : _layer_inactive;
}

std::optional<std::reference_wrapper<PSD::Layer>> MenuPageItem::getCurrentLayer() const
{
   if (const auto layer = getLayer())
   {
      return layer->get().getLayer();
   }
   return std::nullopt;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageItem::getActiveLayer() const
{
   return _layer_active;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageItem::getInactiveLayer() const
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
