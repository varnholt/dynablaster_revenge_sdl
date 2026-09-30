#pragma once

#include <switch.h>
#include <SDL3/SDL.h>
#include <array>
#include <string>

class GameDrawable;
class MenuDrawable;
class MenuMouseCursor;

class SwitchPlatform
{
public:
   bool initialize();
   ~SwitchPlatform();
   void updateInput(bool in_game, GameDrawable& game, MenuDrawable& menu, MenuMouseCursor& cursor);

private:
   void editText(MenuDrawable& menu);
   bool _romfs = false;
   bool _sockets = false;
   bool _nifm = false;
   bool _in_game = false;
   PadState _pad{};
   std::array<SDL_Keycode, 10> _held_keys{};
   float _cursor_x = 960.0f;
   float _cursor_y = 540.0f;
   uint64_t _last_tick = 0;
};
