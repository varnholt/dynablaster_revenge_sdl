#include "menupagelabelitem.h"
#include "fontpool.h"

MenuPageLabelItem::MenuPageLabelItem()
{
   _page_item_type = PageItemTypeLabel;
}

void MenuPageLabelItem::initialize()
{
   _font = FontPool::Instance().get(_font_name);
}

void MenuPageLabelItem::draw()
{
   MenuPageItem::draw();

   if (isVisible())
   {
      if (_color.isValid())
      {
         _font->get().setColor(_color.redF(), _color.greenF(), _color.blueF(), _alpha / 255.0f);
      }
      else
      {
         _font->get().setColor(1.0f, 1.0f, 1.0f, _alpha / 255.0f);
      }

      _font->get().buildVertices(
         _scale,
         _text.c_str(),
         _layer_active->get().getLeft() + _font_x_offset,
         _layer_active->get().getBottom() + _font_y_offset,
         _center_width,
         _center_height
      );

      _font->get().draw();
   }
}

void MenuPageLabelItem::setFontXOffset(int x_offset)
{
   _font_x_offset = x_offset;
}

void MenuPageLabelItem::setFontYOffset(int y_offset)
{
   _font_y_offset = y_offset;
}

void MenuPageLabelItem::setMaxChars(int max_chars)
{
   _max_chars = max_chars;
}

void MenuPageLabelItem::setColor(const Color& color)
{
   _color = color;
}

void MenuPageLabelItem::setAlpha(int alpha)
{
   _alpha = alpha;
}

void MenuPageLabelItem::setCenterWidth(float width)
{
   _center_width = width;
}

void MenuPageLabelItem::setCenterHeight(float height)
{
   _center_height = height;
}

void MenuPageLabelItem::setScale(float scale)
{
   _scale = scale;
}

void MenuPageLabelItem::setText(const std::string& text)
{
   _text = text;
}

std::string MenuPageLabelItem::getText() const
{
   return _text;
}

void MenuPageLabelItem::setFontName(const std::string& font_name)
{
   _font_name = font_name;
}
