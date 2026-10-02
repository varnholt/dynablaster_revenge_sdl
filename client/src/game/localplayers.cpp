#include "localplayers.h"

#include "bombermanclient.h"
#include "gamesettings.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>

namespace
{
uint8_t keysForButtons(uint32_t buttons)
{
   constexpr std::array<std::pair<uint32_t, uint8_t>, 5> key_map{{
      {ControllerInput::ButtonUp, Constants::KeyUp},
      {ControllerInput::ButtonDown, Constants::KeyDown},
      {ControllerInput::ButtonLeft, Constants::KeyLeft},
      {ControllerInput::ButtonRight, Constants::KeyRight},
      {ControllerInput::ButtonBomb, Constants::KeyBomb},
   }};

   uint8_t keys = 0;
   for (const auto& [button, key] : key_map)
   {
      if (buttons & button)
      {
         keys |= key;
      }
   }
   return keys;
}
}  // namespace

LocalPlayers::LocalPlayers(ControllerInput& controller_input, BombermanClient& client)
    : _controller_input(controller_input), _client(client)
{
   // a player whose controller is unplugged leaves the game
   _device_removed_connection = _controller_input.deviceRemovedSignal.connect([this](ControllerInput::Id id) { removeController(id); });
}

LocalPlayers::~LocalPlayers()
{
   _controller_input.deviceRemovedSignal.disconnect(_device_removed_connection);
   removeAll();
}

void LocalPlayers::add(const Player& player)
{
   if (_slots.size() >= max_local_players || _client.getGameId() == -1)
   {
      return;
   }

   Slot slot;
   slot.controller = player.controller;
   slot.client = std::make_unique<LocalPlayerClient>(_client.getHost(), player.nick, _client.getGameId(), player.color);

   LocalPlayerClient* client = slot.client.get();
   client->joinedSignal.connect([this](int32_t) { updateLocalPlayerIds(); });
   client->joinFailedSignal.connect([this, client]() { Timer::singleShot(0, [this, client]() { remove(client); }); });
   if (player.controller)
   {
      const ControllerInput::Id controller = *player.controller;
      client->rumbleSignal.connect([this, controller](float intensity, int32_t duration_ms)
                                   { _controller_input.rumble(controller, intensity, duration_ms); });
      _controller_input.setAssigned(controller, true);
   }

   _slots.push_back(std::move(slot));
}

void LocalPlayers::exclude(ControllerInput::Id controller)
{
   _excluded.insert(controller);
   _controller_input.setAssigned(controller, true);
}

void LocalPlayers::remove(const LocalPlayerClient* client)
{
   const auto it = std::ranges::find_if(_slots, [client](const Slot& slot) { return slot.client.get() == client; });
   if (it == _slots.end())
   {
      return;
   }

   if (it->controller)
   {
      _controller_input.setAssigned(*it->controller, false);
   }
   _slots.erase(it);
   updateLocalPlayerIds();
}

void LocalPlayers::removeController(ControllerInput::Id controller)
{
   _excluded.erase(controller);

   const auto it = std::ranges::find(_slots, std::optional(controller), &Slot::controller);
   if (it != _slots.end())
   {
      remove(it->client.get());
   }
}

void LocalPlayers::removeAll()
{
   for (const auto& slot : _slots)
   {
      if (slot.controller)
      {
         _controller_input.setAssigned(*slot.controller, false);
      }
   }
   _slots.clear();

   for (const ControllerInput::Id controller : _excluded)
   {
      _controller_input.setAssigned(controller, false);
   }
   _excluded.clear();

   updateLocalPlayerIds();
}

uint8_t LocalPlayers::readKeyboard() const
{
   const auto* controls = GameSettings::getInstance()->getControllerSettings();
   const std::array<std::pair<SDL_Keycode, uint8_t>, 5> key_map{{
      {controls->getUpKey(), Constants::KeyUp},
      {controls->getDownKey(), Constants::KeyDown},
      {controls->getLeftKey(), Constants::KeyLeft},
      {controls->getRightKey(), Constants::KeyRight},
      {controls->getBombKey(), Constants::KeyBomb},
   }};

   const bool* state = SDL_GetKeyboardState(nullptr);
   uint8_t keys = 0;
   for (const auto& [keycode, key] : key_map)
   {
      if (state[SDL_GetScancodeFromKey(keycode, nullptr)])
      {
         keys |= key;
      }
   }
   return keys;
}

void LocalPlayers::update(bool in_game)
{
   for (const auto& slot : _slots)
   {
      uint8_t keys = 0;
      if (in_game)
      {
         keys = slot.controller ? keysForButtons(_controller_input.getButtons(*slot.controller)) : readKeyboard();
      }
      slot.client->setKeys(keys);
   }
}

size_t LocalPlayers::getCount() const
{
   return _slots.size();
}

bool LocalPlayers::isJoining() const
{
   return std::ranges::any_of(_slots, [](const Slot& slot) { return !slot.client->isJoined(); });
}

bool LocalPlayers::isKeyboardAssigned() const
{
   return std::ranges::any_of(_slots, [](const Slot& slot) { return !slot.controller; });
}

void LocalPlayers::updateLocalPlayerIds()
{
   std::vector<int32_t> ids;
   for (const auto& slot : _slots)
   {
      if (slot.client->isJoined())
      {
         ids.push_back(slot.client->getPlayerId());
      }
   }
   _client.setLocalPlayerIds(ids);
}
