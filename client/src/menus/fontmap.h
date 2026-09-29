#pragma once

#include "bitmapfont.h"

#include <array>

class MenuFont
{
public:
   static std::array<BitmapFont::Parameter, 129> _menu_chars;
};
