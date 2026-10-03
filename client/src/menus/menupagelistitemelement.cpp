#include "menupagelistitemelement.h"

#include "bitmapfont.h"
#include "framework/gldevice.h"

#include <cmath>

MenuPageListItemElement::MenuPageListItemElement(MenuPageListItem& parent) : _parent(parent)
{
   _page_item_type = PageItemTypeListElement;
}

MenuPageListItemElement::~MenuPageListItemElement() = default;

MenuPageListItem& MenuPageListItemElement::getParent() const
{
   return _parent;
}

void MenuPageListItemElement::initialize()
{
   MenuPageTextEditItem::initialize();
}

void MenuPageListItemElement::draw(float x, float y, float opacity)
{
   const Color& rgb = getColor();
   _font->get().setColor(rgb.red() / 255.0f, rgb.green() / 255.0f, rgb.blue() / 255.0f, opacity);

   // draw item data
   _font->get().buildVertices(_scale, _text.c_str(), _x + x, _y + y + _height, -1, _height);

   float r = 0.0f;
   float g = 0.0f;
   float b = 0.0f;
   float a = 0.0f;
   if (_outline_color.isValid())
   {
      _font->get().getOutlineColor(r, g, b, a);
      _font->get().setOutlineColor(_outline_color.redF(), _outline_color.greenF(), _outline_color.blueF(), _outline_color.alphaF());
   }

   _font->get().draw();

   if (_outline_color.isValid())
   {
      _font->get().setOutlineColor(r, g, b, a);
   }
}

void MenuPageListItemElement::setFocus(bool focus)
{
   // focus lost, then fade out
   if (_focussed && !focus && !_active)
   {
      _fade_out = true;
      _focus_out_time.restart();
   }

   MenuPageTextEditItem::setFocus(focus);
}

void MenuPageListItemElement::setActive(bool active)
{
   // active flag lost, then fade out
   if (_active && !active)
   {
      _active = true;
      _focus_out_time.restart();
   }

   MenuPageTextEditItem::setActive(active);
}

void MenuPageListItemElement::setIndex(int index)
{
   _index = index;
}

void MenuPageListItemElement::setHeight(int height)
{
   _height = height;
}

void MenuPageListItemElement::setWidth(int width)
{
   _width = width;
}

int MenuPageListItemElement::getHeight() const
{
   return _height;
}

int MenuPageListItemElement::getWidth() const
{
   return _width;
}

void MenuPageListItemElement::setX(float x)
{
   _x = x;
}

void MenuPageListItemElement::setY(float y)
{
   _y = y;
}

float MenuPageListItemElement::getX() const
{
   return _x;
}

float MenuPageListItemElement::getY() const
{
   return _y;
}

std::vector<Vertex> MenuPageListItemElement::getBoundingRectVertices(float x, float y)
{
   return {
      Vertex(x + _x, y + _y + _height, 0.0f, 1.0f),
      Vertex(x + _x + _width, y + _y + _height, 1.0f, 1.0f),
      Vertex(x + _x + _width, y + _y, 1.0f, 0.0f),
      Vertex(x + _x, y + _y, 0.0f, 0.0f),
   };
}

void MenuPageListItemElement::stopFadeOut()
{
   _fade_out = false;
}

bool MenuPageListItemElement::isFadingOut()
{
   return _fade_out;
}

float MenuPageListItemElement::getFadeOutValue()
{
   float value = 0.0f;

   if (getFocusOutTime().elapsed() < 500)
   {
      value = std::cos(_focus_out_time.elapsed() * 0.01);
   }

   return value;
}

void MenuPageListItemElement::setOverrideAlpha(bool override_alpha)
{
   _override_alpha = override_alpha;
}

bool MenuPageListItemElement::isOverrideAlphaActive() const
{
   return _override_alpha;
}

const FrameTimer& MenuPageListItemElement::getFocusOutTime() const
{
   return _focus_out_time;
}
