#pragma once

// base
#include "menupageitem.h"

// math
#include "math/color.h"

// forward declarations
class BitmapFont;

class MenuPageLabelItem : public MenuPageItem
{
public:
   MenuPageLabelItem();

   // main

   virtual void draw();

   virtual void initialize();

   std::string getText() const;

   void setFontName(const std::string& fontName);

   void setText(const std::string&);

   void setFontXOffset(int xOffset);

   void setFontYOffset(int yOffset);

   void setScale(float scale);

   void setMaxChars(int maxChars);

   void setColor(const Color& color);

   void setAlpha(int alpha);

   void setCenterWidth(float width);

   void setCenterHeight(float height);

protected:
   std::string mFontName;

   BitmapFont* mFont;

   std::string mText;

   int mFontXOffset;

   int mFontYOffset;

   int mMaxChars;

   float mScale;

   Color mColor;

   int mAlpha;

   float mCenterWidth;

   float mCenterHeight;
};
