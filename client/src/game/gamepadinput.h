#pragma once

#include "gamesignal.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class GameDrawable;

/// \brief SDL3 gamepads incl. hotplug. in game the pads synthesize the keyboard's keymap, the
/// menu side is MenuJoystickHandler.
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
      ButtonBomb = 0x0010,  //!< a, b, x or y
      ButtonShoulderLeft = 0x0020,
      ButtonShoulderRight = 0x0040,
      ButtonStart = 0x0080,
      ButtonLast = ButtonStart,
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

   /// \brief reads every device and fires the button signals on changes
   void poll();

   /// \brief poll() plus feeding the game while it is visible
   void update(bool in_game, GameDrawable& game);

   /// \brief rumbles every device, intensity 0..1
   void rumble(float intensity, int32_t duration_ms);

   std::vector<DeviceInfo> getDevices() const;
   uint32_t getButtons(SDL_JoystickID id) const;

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
   };

   void addDevice(SDL_JoystickID id);
   void removeDevice(SDL_JoystickID id);
   uint32_t readButtons(const Device& device) const;
   void refreshButtons(Device& device);
   void updateGame(bool in_game, GameDrawable& game);

   bool _initialized = false;
   std::unordered_map<SDL_JoystickID, Device> _devices;
   std::vector<SDL_Keycode> _held_keys;
};
