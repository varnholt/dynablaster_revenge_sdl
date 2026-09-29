#pragma once

#include "menupageitem.h"

#include "math/color.h"

class BitmapFont;

class MenuPageLabelItem : public MenuPageItem
{
public:
   MenuPageLabelItem();

   void draw() override;

   void initialize() override;

   std::string getText() const;

   void setFontName(const std::string& font_name);

   void setText(const std::string&);

   void setFontXOffset(int x_offset);

   void setFontYOffset(int y_offset);

   void setScale(float scale);

   void setMaxChars(int max_chars);

   void setColor(const Color& color);

   void setAlpha(int alpha);

   void setCenterWidth(float width);

   void setCenterHeight(float height);

protected:
   std::string _font_name;

   // non-owning, fonts belong to the FontPool
   BitmapFont* _font = nullptr;

   std::string _text;

   int _font_x_offset = 0;
   int _font_y_offset = 0;
   int _max_chars = 255;

   float _scale = 0.0f;

   Color _color;

   int _alpha = 255;

   float _center_width = -1.0f;
   float _center_height = -1.0f;
};
