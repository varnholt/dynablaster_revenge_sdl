#include "gamepadinput.h"

#include "framework/keyevent.h"
#include "game/gamedrawable.h"
#include "game/gamesettings.h"
#include "menus/menu.h"
#include "menus/menudrawable.h"
#include "menus/menumousecursor.h"
#include "menus/menupage.h"
#include "menus/menupageitem.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
constexpr float cursor_speed = 1200.0f;  // page pixels per second at full deflection

constexpr std::array<std::pair<SDL_GamepadButton, uint32_t>, 12> button_map{{
   {SDL_GAMEPAD_BUTTON_DPAD_UP, GamepadInput::ButtonUp},
   {SDL_GAMEPAD_BUTTON_DPAD_DOWN, GamepadInput::ButtonDown},
   {SDL_GAMEPAD_BUTTON_DPAD_LEFT, GamepadInput::ButtonLeft},
   {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, GamepadInput::ButtonRight},
   {SDL_GAMEPAD_BUTTON_SOUTH, GamepadInput::ButtonSouth},
   {SDL_GAMEPAD_BUTTON_EAST, GamepadInput::ButtonEast},
   {SDL_GAMEPAD_BUTTON_WEST, GamepadInput::ButtonWest},
   {SDL_GAMEPAD_BUTTON_NORTH, GamepadInput::ButtonNorth},
   {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, GamepadInput::ButtonShoulderLeft},
   {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, GamepadInput::ButtonShoulderRight},
   {SDL_GAMEPAD_BUTTON_BACK, GamepadInput::ButtonBack},
   {SDL_GAMEPAD_BUTTON_START, GamepadInput::ButtonStart},
}};

float normalizedAxis(int16_t value)
{
   return std::clamp(static_cast<float>(value) / 32767.0f, -1.0f, 1.0f);
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
   }

   // already connected pads arrive as SDL_EVENT_GAMEPAD_ADDED, too
   _last_tick = SDL_GetTicks();
   return _initialized;
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

   SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
   if (!gamepad)
   {
      SDL_Log("Failed to open gamepad %u: %s", id, SDL_GetError());
      return;
   }

   Device device;
   device.gamepad = gamepad;
   device.info.id = id;
   const char* name = SDL_GetGamepadName(gamepad);
   device.info.name = name ? name : "gamepad";
   std::array<char, 33> guid{};
   SDL_GUIDToString(SDL_GetGamepadGUIDForID(id), guid.data(), static_cast<int>(guid.size()));
   device.info.guid = guid.data();
   _devices.emplace(id, std::move(device));

   SDL_Log("Gamepad %u connected: %s", id, name ? name : "unknown");
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
   for (uint32_t bit = 1; bit <= ButtonStart; bit <<= 1)
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

void GamepadInput::refreshButtons(Device& device)
{
   uint32_t buttons = 0;
   for (const auto& [sdl_button, button] : button_map)
   {
      if (SDL_GetGamepadButton(device.gamepad, sdl_button))
      {
         buttons |= button;
      }
   }

   const float threshold = static_cast<float>(GameSettings::getInstance()->getControllerSettings()->getAnalogueThreshold()) / 32767.0f;
   device.stick_x = normalizedAxis(SDL_GetGamepadAxis(device.gamepad, SDL_GAMEPAD_AXIS_LEFTX));
   device.stick_y = normalizedAxis(SDL_GetGamepadAxis(device.gamepad, SDL_GAMEPAD_AXIS_LEFTY));

   if (std::abs(device.stick_x) < threshold)
   {
      device.stick_x = 0.0f;
   }
   if (std::abs(device.stick_y) < threshold)
   {
      device.stick_y = 0.0f;
   }

   if (device.stick_x < 0.0f)
   {
      buttons |= ButtonLeft;
   }
   else if (device.stick_x > 0.0f)
   {
      buttons |= ButtonRight;
   }
   if (device.stick_y < 0.0f)
   {
      buttons |= ButtonUp;
   }
   else if (device.stick_y > 0.0f)
   {
      buttons |= ButtonDown;
   }

   if (SDL_GetGamepadAxis(device.gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) > 16384)
   {
      buttons |= ButtonTriggerLeft;
   }

   const uint32_t changed = buttons ^ device.buttons;
   device.buttons = buttons;

   for (uint32_t bit = 1; bit <= ButtonStart; bit <<= 1)
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

void GamepadInput::update(bool in_game, GameDrawable& game, MenuDrawable& menu, MenuMouseCursor& cursor)
{
   poll();

   updateGame(in_game, game);

   if (in_game != _in_game)
   {
      menu.mouseReleaseEvent();
      cursor.mouseReleaseEvent();
      _in_game = in_game;
   }

   // menu buttons only fire on edges of the combined state of all pads
   uint32_t buttons = 0;
   for (const auto& [id, device] : _devices)
   {
      buttons |= device.buttons;
   }
   const uint32_t pressed = buttons & ~_menu_buttons;
   const uint32_t released = _menu_buttons & ~buttons;
   _menu_buttons = buttons;

   if (!in_game)
   {
      updateMenu(menu, cursor, pressed, released);
   }

   _last_tick = SDL_GetTicks();
}

void GamepadInput::updateGame(bool in_game, GameDrawable& game)
{
   // all pads share the local player for now, keys are synthesized from the combined state
   std::vector<SDL_Keycode> keys;
   if (in_game)
   {
      uint32_t buttons = 0;
      for (const auto& [id, device] : _devices)
      {
         buttons |= device.buttons;
      }

      const auto* controls = GameSettings::getInstance()->getControllerSettings();
      const std::array<std::pair<uint32_t, SDL_Keycode>, 10> key_map{{
         {ButtonUp, controls->getUpKey()},
         {ButtonDown, controls->getDownKey()},
         {ButtonLeft, controls->getLeftKey()},
         {ButtonRight, controls->getRightKey()},
         {ButtonSouth | ButtonEast | ButtonWest | ButtonNorth, controls->getBombKey()},
         {ButtonShoulderLeft, controls->getZoomOutKey()},
         {ButtonShoulderRight, controls->getZoomInKey()},
         {ButtonTriggerLeft, SDLK_TAB},
         {ButtonBack, SDLK_ESCAPE},
         {ButtonStart, SDLK_F10},
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

void GamepadInput::updateMenu(MenuDrawable& menu, MenuMouseCursor& cursor, uint32_t pressed, uint32_t released)
{
   const float dt = std::min(static_cast<float>(SDL_GetTicks() - _last_tick) * 0.001f, 0.05f);

   // the d-pad moves at full speed, the stick proportionally
   float dx = 0.0f;
   float dy = 0.0f;
   for (const auto& [id, device] : _devices)
   {
      const uint32_t dpad = device.buttons;
      const float x = (dpad & ButtonLeft) && device.stick_x == 0.0f    ? -1.0f
                      : (dpad & ButtonRight) && device.stick_x == 0.0f ? 1.0f
                                                                       : device.stick_x;
      const float y = (dpad & ButtonUp) && device.stick_y == 0.0f     ? -1.0f
                      : (dpad & ButtonDown) && device.stick_y == 0.0f ? 1.0f
                                                                      : device.stick_y;
      dx = std::abs(x) > std::abs(dx) ? x : dx;
      dy = std::abs(y) > std::abs(dy) ? y : dy;
   }

   if (dx != 0.0f || dy != 0.0f)
   {
      _cursor_x = std::clamp(_cursor_x + dx * dt * cursor_speed, 0.0f, 1919.0f);
      _cursor_y = std::clamp(_cursor_y + dy * dt * cursor_speed, 0.0f, 1079.0f);
      menu.mouseMoveEvent(static_cast<int>(_cursor_x), static_cast<int>(_cursor_y));
      cursor.mouseMoveEvent(static_cast<int>(_cursor_x), static_cast<int>(_cursor_y));
   }

   const int x = static_cast<int>(_cursor_x);
   const int y = static_cast<int>(_cursor_y);
   if (pressed & ButtonSouth)
   {
      menu.mousePressEvent(x, y);
      cursor.mousePressEvent(x, y);
   }
   if (released & ButtonSouth)
   {
      menu.mouseReleaseEvent();
      cursor.mouseReleaseEvent();
   }
   if (pressed & ButtonEast)
   {
      clickBackButton(menu);
   }
}

void GamepadInput::setCursorPosition(int x, int y)
{
   _cursor_x = static_cast<float>(x);
   _cursor_y = static_cast<float>(y);
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

bool GamepadInput::clickBackButton(MenuDrawable& menu)
{
   auto* page = menu.getMenu()->getCurrentPage();
   if (!page)
   {
      return false;
   }

   for (const auto& item : page->getPageItems())
   {
      auto* layer = item->getCurrentLayer();
      if (!layer || !item->isVisible() || !item->isEnabled())
      {
         continue;
      }

      const std::string name = layer->getName();
      if (name.starts_with("button_back") || name.starts_with("button_cancel") || name.starts_with("button_leave"))
      {
         const int x = layer->getLeft() + layer->getWidth() / 2;
         const int y = layer->getTop() + layer->getHeight() / 2;
         menu.mouseMoveEvent(x, y);
         menu.mousePressEvent(x, y);
         menu.mouseReleaseEvent();
         return true;
      }
   }

   return false;
}
