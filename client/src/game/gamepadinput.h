#pragma once

#include "gamesignal.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class GameDrawable;
class MenuDrawable;
class MenuMouseCursor;

/// \brief SDL3 gamepads incl. hotplug. drives the game via synthesized key events (same keymap as
/// the keyboard) and the menu via the cursor, like SwitchPlatform does for the Switch pad.
class GamepadInput
{
public:
   /// \brief logical buttons; directions merge the d-pad and the left stick
   enum Button : uint32_t
   {
      ButtonUp = 0x0001,
      ButtonDown = 0x0002,
      ButtonLeft = 0x0004,
      ButtonRight = 0x0008,
      ButtonSouth = 0x0010,
      ButtonEast = 0x0020,
      ButtonWest = 0x0040,
      ButtonNorth = 0x0080,
      ButtonShoulderLeft = 0x0100,
      ButtonShoulderRight = 0x0200,
      ButtonTriggerLeft = 0x0400,
      ButtonBack = 0x0800,
      ButtonStart = 0x1000,
   };

   struct DeviceInfo
   {
      SDL_JoystickID id = 0;
      std::string name;
      std::string guid;
   };

   GamepadInput() = default;
   GamepadInput(const GamepadInput&) = delete;
   GamepadInput& operator=(const GamepadInput&) = delete;
   ~GamepadInput();

   bool initialize();
   void handleEvent(const SDL_Event& event);

   /// rief reads every pad and fires the button signals on changes
   void poll();

   /// rief poll() plus driving the game or the menu
   void update(bool in_game, GameDrawable& game, MenuDrawable& menu, MenuMouseCursor& cursor);

   /// \brief keeps the pad-driven cursor where the real mouse left it
   void setCursorPosition(int x, int y);

   std::vector<DeviceInfo> getDevices() const;
   uint32_t getButtons(SDL_JoystickID id) const;

   /// \brief clicks the current page's back/cancel/leave button, if there is one
   static bool clickBackButton(MenuDrawable& menu);

   Signal<SDL_JoystickID> deviceAddedSignal;
   Signal<SDL_JoystickID> deviceRemovedSignal;
   Signal<SDL_JoystickID, Button> buttonPressedSignal;
   Signal<SDL_JoystickID, Button> buttonReleasedSignal;

private:
   struct Device
   {
      SDL_Gamepad* gamepad = nullptr;
      DeviceInfo info;
      uint32_t buttons = 0;
      float stick_x = 0.0f;
      float stick_y = 0.0f;
   };

   void addDevice(SDL_JoystickID id);
   void removeDevice(SDL_JoystickID id);
   void refreshButtons(Device& device);
   void updateGame(bool in_game, GameDrawable& game);
   void updateMenu(MenuDrawable& menu, MenuMouseCursor& cursor, uint32_t pressed, uint32_t released);

   bool _initialized = false;
   std::unordered_map<SDL_JoystickID, Device> _devices;
   uint32_t _menu_buttons = 0;
   std::vector<SDL_Keycode> _held_keys;
   bool _in_game = false;
   float _cursor_x = 960.0f;
   float _cursor_y = 540.0f;
   uint64_t _last_tick = 0;
};
