#include "gamefonts.h"

#include "bitmapfont.h"
#include "fontmap.h"
#include "fontpool.h"

void registerGameFonts()
{
   auto* font_default =
      new BitmapFont("data/fonts/font", MenuFont::_menu_chars.data(), 2.1f, 3.0f, 32.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.05f, -0.025f);

   auto* font_lounge =
      new BitmapFont("data/fonts/font", MenuFont::_menu_chars.data(), 2.2f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.1f, 0.05f, -0.025f);

   // tab overlay (GamePlayerNameDisplay)
   auto* font_outlined =
      new BitmapFont("data/fonts/font", MenuFont::_menu_chars.data(), 2.0f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.35f, 0.03f, -0.025f);

   // win/trophy screen (GameWinDrawable) headline and scoreboard rows
   auto* font_large =
      new BitmapFont("data/fonts/font", MenuFont::_menu_chars.data(), 2.0f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.04f, 0.0025f, -0.01f);

   auto* font_large_outlined =
      new BitmapFont("data/fonts/font", MenuFont::_menu_chars.data(), 2.0f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.15f, 0.03f, -0.025f);

   FontPool::Instance()->add("default", font_default);
   FontPool::Instance()->add("lounge", font_lounge);
   FontPool::Instance()->add("outlined", font_outlined);
   FontPool::Instance()->add("large", font_large);
   FontPool::Instance()->add("large-outlined", font_large_outlined);
}
