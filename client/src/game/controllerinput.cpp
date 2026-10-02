#include "controllerinput.h"

#include "framework/keyevent.h"
#include "game/gamedrawable.h"
#include "game/gamesettings.h"

#include <algorithm>
#include <array>

namespace
{
constexpr std::array<std::pair<SDL_GamepadButton, uint32_t>, 11> button_map{{
   {SDL_GAMEPAD_BUTTON_DPAD_UP, ControllerInput::ButtonUp},
   {SDL_GAMEPAD_BUTTON_DPAD_DOWN, ControllerInput::ButtonDown},
   {SDL_GAMEPAD_BUTTON_DPAD_LEFT, ControllerInput::ButtonLeft},
   {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, ControllerInput::ButtonRight},
   {SDL_GAMEPAD_BUTTON_SOUTH, ControllerInput::ButtonBomb},
   {SDL_GAMEPAD_BUTTON_EAST, ControllerInput::ButtonBomb},
   {SDL_GAMEPAD_BUTTON_WEST, ControllerInput::ButtonBomb},
   {SDL_GAMEPAD_BUTTON_NORTH, ControllerInput::ButtonBomb},
   {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, ControllerInput::ButtonShoulderLeft},
   {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, ControllerInput::ButtonShoulderRight},
   {SDL_GAMEPAD_BUTTON_START, ControllerInput::ButtonStart},
}};
}  // namespace

ControllerInput::~ControllerInput()
{
   for (auto& [id, device] : _devices)
   {
      SDL_CloseGamepad(device.controller);
   }

   if (_initialized)
   {
      SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
   }
}

bool ControllerInput::initialize()
{
   if (!GameSettings::getInstance()->getDevelopmentSettings()->isControllersEnabled())
   {
      return false;
   }

   _initialized = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
   if (!_initialized)
   {
      SDL_Log("Failed to initialize controllers: %s", SDL_GetError());
      return false;
   }

   // already connected controllers arrive as SDL_EVENT_GAMEPAD_ADDED, too
   return true;
}

void ControllerInput::handleEvent(const SDL_Event& event)
{
   if (event.type == SDL_EVENT_GAMEPAD_ADDED)
   {
      addDevice(event.gdevice.which);
   }
   else if (event.type == SDL_EVENT_GAMEPAD_REMOVED)
   {
      removeDevice(event.gdevice.which);
   }
}

void ControllerInput::addDevice(Id id)
{
   if (_devices.contains(id))
   {
      return;
   }

   Device device;
   device.controller = SDL_OpenGamepad(id);
   if (!device.controller)
   {
      SDL_Log("Failed to open controller %u: %s", id, SDL_GetError());
      return;
   }

   device.info.id = id;
   const char* name = SDL_GetGamepadName(device.controller);
   device.info.name = name ? name : "controller";
   std::array<char, 33> guid{};
   SDL_GUIDToString(SDL_GetGamepadGUIDForID(id), guid.data(), static_cast<int>(guid.size()));
   device.info.guid = guid.data();

   SDL_Log("Controller %u connected: %s", id, device.info.name.c_str());
   _devices.emplace(id, std::move(device));
   deviceAddedSignal(id);
}

void ControllerInput::removeDevice(Id id)
{
   auto it = _devices.find(id);
   if (it == _devices.end())
   {
      return;
   }

   // release whatever was held so nothing gets stuck
   const uint32_t held = it->second.buttons;
   for (uint32_t bit = 1; bit <= ButtonLast; bit <<= 1)
   {
      if (held & bit)
      {
         buttonReleasedSignal(id, static_cast<Button>(bit));
      }
   }

   SDL_CloseGamepad(it->second.controller);
   _devices.erase(it);
   _assigned.erase(id);

   SDL_Log("Controller %u disconnected", id);
   deviceRemovedSignal(id);
}

uint32_t ControllerInput::readButtons(const Device& device) const
{
   uint32_t buttons = 0;
   for (const auto& [sdl_button, button] : button_map)
   {
      if (SDL_GetGamepadButton(device.controller, sdl_button))
      {
         buttons |= button;
      }
   }

   // the left stick unless calibration picked another one
   const auto* controls = GameSettings::getInstance()->getControllerSettings();
   const auto axis = [](int value) { return static_cast<SDL_GamepadAxis>(std::clamp(value, 0, SDL_GAMEPAD_AXIS_COUNT - 1)); };
   const int32_t threshold = controls->getAnalogueThreshold();
   const int16_t x = SDL_GetGamepadAxis(device.controller, axis(controls->getAnalogueAxis1()));
   const int16_t y = SDL_GetGamepadAxis(device.controller, axis(controls->getAnalogueAxis2()));
   if (x < -threshold)
   {
      buttons |= ButtonLeft;
   }
   else if (x > threshold)
   {
      buttons |= ButtonRight;
   }
   if (y < -threshold)
   {
      buttons |= ButtonUp;
   }
   else if (y > threshold)
   {
      buttons |= ButtonDown;
   }

   return buttons;
}

void ControllerInput::refreshButtons(Device& device)
{
   const uint32_t buttons = readButtons(device);
   const uint32_t changed = buttons ^ device.buttons;
   device.buttons = buttons;

   for (uint32_t bit = 1; bit <= ButtonLast; bit <<= 1)
   {
      if (changed & bit)
      {
         if (buttons & bit)
         {
            buttonPressedSignal(device.info.id, static_cast<Button>(bit));
         }
         else
         {
            buttonReleasedSignal(device.info.id, static_cast<Button>(bit));
         }
      }
   }
}

void ControllerInput::poll()
{
   for (auto& [id, device] : _devices)
   {
      refreshButtons(device);
   }
}

void ControllerInput::update(bool in_game, GameDrawable& game)
{
   poll();
   updateGame(in_game, game);
}

void ControllerInput::updateGame(bool in_game, GameDrawable& game)
{
   // the unassigned controllers share the main player, keys are synthesized from their combined state
   std::vector<SDL_Keycode> keys;
   if (in_game)
   {
      uint32_t buttons = 0;
      for (const auto& [id, device] : _devices)
      {
         if (!_assigned.contains(id))
         {
            buttons |= device.buttons;
         }
      }

      const auto* controls = GameSettings::getInstance()->getControllerSettings();
      const std::array<std::pair<uint32_t, SDL_Keycode>, 7> key_map{{
         {ButtonUp, controls->getUpKey()},
         {ButtonDown, controls->getDownKey()},
         {ButtonLeft, controls->getLeftKey()},
         {ButtonRight, controls->getRightKey()},
         {ButtonBomb, controls->getBombKey()},
         {ButtonShoulderLeft, controls->getZoomOutKey()},
         {ButtonShoulderRight, controls->getZoomInKey()},
      }};

      for (const auto& [mask, key] : key_map)
      {
         if (buttons & mask)
         {
            keys.push_back(key);
         }
      }
   }

   for (const auto key : _held_keys)
   {
      if (std::ranges::find(keys, key) == keys.end())
      {
         game.keyReleaseEvent(KeyEvent(key, {}, false));
      }
   }
   for (const auto key : keys)
   {
      if (std::ranges::find(_held_keys, key) == _held_keys.end())
      {
         game.keyPressEvent(KeyEvent(key, {}, false));
      }
   }
   _held_keys = std::move(keys);
}

void ControllerInput::rumble(float intensity, int32_t duration_ms)
{
   for (const auto& [id, device] : _devices)
   {
      if (!_assigned.contains(id))
      {
         rumble(id, intensity, duration_ms);
      }
   }
}

void ControllerInput::rumble(Id id, float intensity, int32_t duration_ms)
{
   const auto it = _devices.find(id);
   if (it != _devices.end())
   {
      const auto strength = static_cast<uint16_t>(std::clamp(intensity, 0.0f, 1.0f) * 0xffff);
      SDL_RumbleGamepad(it->second.controller, strength, strength, static_cast<uint32_t>(duration_ms));
   }
}

void ControllerInput::setAssigned(Id id, bool assigned)
{
   if (assigned)
   {
      _assigned.insert(id);
   }
   else
   {
      _assigned.erase(id);
   }
}

bool ControllerInput::isAssigned(Id id) const
{
   return _assigned.contains(id);
}

std::vector<ControllerInput::DeviceInfo> ControllerInput::getDevices() const
{
   std::vector<DeviceInfo> devices;
   devices.reserve(_devices.size());
   for (const auto& [id, device] : _devices)
   {
      devices.push_back(device.info);
   }

   // stable order: connection order, SDL hands out increasing instance ids
   std::ranges::sort(devices, {}, &DeviceInfo::id);
   return devices;
}

uint32_t ControllerInput::getButtons(Id id) const
{
   const auto it = _devices.find(id);
   return it != _devices.end() ? it->second.buttons : 0;
}

std::optional<ControllerInput::State> ControllerInput::getState(Id id) const
{
   const auto it = _devices.find(id);
   if (it == _devices.end())
   {
      return std::nullopt;
   }

   State state;
   for (int32_t button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++)
   {
      state.buttons[button] = SDL_GetGamepadButton(it->second.controller, static_cast<SDL_GamepadButton>(button));
   }

   for (int32_t axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++)
   {
      state.axes[axis] = SDL_GetGamepadAxis(it->second.controller, static_cast<SDL_GamepadAxis>(axis));
   }

   return state;
}
