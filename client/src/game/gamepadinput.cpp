#include "gamepadinput.h"

#include "framework/keyevent.h"
#include "game/gamedrawable.h"
#include "game/gamesettings.h"

#include <algorithm>
#include <array>

namespace
{
uint32_t axisButtons(int16_t x, int16_t y, int32_t threshold)
{
   uint32_t buttons = 0;
   if (x < -threshold)
   {
      buttons |= GamepadInput::ButtonLeft;
   }
   else if (x > threshold)
   {
      buttons |= GamepadInput::ButtonRight;
   }
   if (y < -threshold)
   {
      buttons |= GamepadInput::ButtonUp;
   }
   else if (y > threshold)
   {
      buttons |= GamepadInput::ButtonDown;
   }
   return buttons;
}
}  // namespace

GamepadInput::~GamepadInput()
{
   for (auto& [id, device] : _devices)
   {
      SDL_CloseGamepad(device.gamepad);
   }

   if (_initialized)
   {
      SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
   }
}

bool GamepadInput::initialize()
{
   if (!GameSettings::getInstance()->getDevelopmentSettings()->isJoysticksEnabled())
   {
      return false;
   }

   _initialized = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
   if (!_initialized)
   {
      SDL_Log("Failed to initialize gamepads: %s", SDL_GetError());
      return false;
   }

   // already connected pads arrive as SDL_EVENT_GAMEPAD_ADDED, too
   return true;
}

void GamepadInput::handleEvent(const SDL_Event& event)
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

void GamepadInput::addDevice(SDL_JoystickID id)
{
   if (_devices.contains(id))
   {
      return;
   }

   Device device;
   device.gamepad = SDL_OpenGamepad(id);
   if (!device.gamepad)
   {
      SDL_Log("Failed to open gamepad %u: %s", id, SDL_GetError());
      return;
   }

   device.info.id = id;
   const char* name = SDL_GetGamepadName(device.gamepad);
   device.info.name = name ? name : "gamepad";
   std::array<char, 33> guid{};
   SDL_GUIDToString(SDL_GetGamepadGUIDForID(id), guid.data(), static_cast<int>(guid.size()));
   device.info.guid = guid.data();

   SDL_Log("Gamepad %u connected: %s", id, device.info.name.c_str());
   _devices.emplace(id, std::move(device));
   deviceAddedSignal(id);
}

void GamepadInput::removeDevice(SDL_JoystickID id)
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

   SDL_CloseGamepad(it->second.gamepad);
   _devices.erase(it);

   SDL_Log("Gamepad %u disconnected", id);
   deviceRemovedSignal(id);
}

uint32_t GamepadInput::readButtons(const Device& device) const
{
   const auto* controls = GameSettings::getInstance()->getControllerSettings();
   const int32_t threshold = controls->getAnalogueThreshold();
   uint32_t buttons = 0;

   SDL_Gamepad* gamepad = device.gamepad;

   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_UP))
   {
      buttons |= ButtonUp;
   }
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_DOWN))
   {
      buttons |= ButtonDown;
   }
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_LEFT))
   {
      buttons |= ButtonLeft;
   }
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_DPAD_RIGHT))
   {
      buttons |= ButtonRight;
   }

   buttons |=
      axisButtons(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX), SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY), threshold);

   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH) || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_EAST) ||
       SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_WEST) || SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_NORTH))
   {
      buttons |= ButtonBomb;
   }
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER))
   {
      buttons |= ButtonShoulderLeft;
   }
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER))
   {
      buttons |= ButtonShoulderRight;
   }
   if (SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_START))
   {
      buttons |= ButtonStart;
   }

   return buttons;
}

void GamepadInput::refreshButtons(Device& device)
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

void GamepadInput::poll()
{
   for (auto& [id, device] : _devices)
   {
      refreshButtons(device);
   }
}

void GamepadInput::update(bool in_game, GameDrawable& game)
{
   poll();
   updateGame(in_game, game);
}

void GamepadInput::updateGame(bool in_game, GameDrawable& game)
{
   // all devices share the local player for now, keys are synthesized from the combined state
   std::vector<SDL_Keycode> keys;
   if (in_game)
   {
      uint32_t buttons = 0;
      for (const auto& [id, device] : _devices)
      {
         buttons |= device.buttons;
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

void GamepadInput::rumble(float intensity, int32_t duration_ms)
{
   const auto strength = static_cast<uint16_t>(std::clamp(intensity, 0.0f, 1.0f) * 0xffff);
   for (const auto& [id, device] : _devices)
   {
      SDL_RumbleGamepad(device.gamepad, strength, strength, static_cast<uint32_t>(duration_ms));
   }
}

std::vector<GamepadInput::DeviceInfo> GamepadInput::getDevices() const
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

uint32_t GamepadInput::getButtons(SDL_JoystickID id) const
{
   const auto it = _devices.find(id);
   return it != _devices.end() ? it->second.buttons : 0;
}
