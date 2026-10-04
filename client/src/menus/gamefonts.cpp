#include "gamefonts.h"

#include "bitmapfont.h"
#include "fontmap.h"
#include "fontpool.h"

#include <memory>
#include <utility>

void registerGameFonts()
{
   auto font_default = std::make_unique<BitmapFont>(
      "data/fonts/font", MenuFont::_menu_chars, 2.1f, 3.0f, 32.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.1f, 0.05f, -0.025f
   );

   auto font_lounge = std::make_unique<BitmapFont>(
      "data/fonts/font", MenuFont::_menu_chars, 2.2f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.1f, 0.05f, -0.025f
   );

   // tab overlay (GamePlayerNameDisplay)
   auto font_outlined = std::make_unique<BitmapFont>(
      "data/fonts/font", MenuFont::_menu_chars, 2.0f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.35f, 0.03f, -0.025f
   );

   // game clock of the HUD (GameStatsDrawable)
   auto font_time = std::make_unique<BitmapFont>(
      "data/fonts/font", MenuFont::_menu_chars, 2.1f, 3.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.15f, 0.03f, -0.025f
   );

   // win/trophy screen (GameWinDrawable) headline and scoreboard rows
   auto font_large = std::make_unique<BitmapFont>(
      "data/fonts/font", MenuFont::_menu_chars, 2.0f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.04f, 0.0025f, -0.01f
   );

   auto font_large_outlined = std::make_unique<BitmapFont>(
      "data/fonts/font", MenuFont::_menu_chars, 2.0f, 4.0f, 32.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.15f, 0.03f, -0.025f
   );

   FontPool::Instance().add("default", std::move(font_default));
   FontPool::Instance().add("lounge", std::move(font_lounge));
   FontPool::Instance().add("outlined", std::move(font_outlined));
   FontPool::Instance().add("time", std::move(font_time));
   FontPool::Instance().add("large", std::move(font_large));
   FontPool::Instance().add("large-outlined", std::move(font_large_outlined));
}
