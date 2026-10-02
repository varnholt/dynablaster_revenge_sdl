#include "localplayers.h"

#include "bombermanclient.h"
#include "constants.h"
#include "gamesettings.h"

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
   _device_removed_connection = _controller_input.deviceRemovedSignal.connect([this](ControllerInput::Id id) { remove(id); });
}

LocalPlayers::~LocalPlayers()
{
   _controller_input.deviceRemovedSignal.disconnect(_device_removed_connection);
   removeAll();
}

bool LocalPlayers::canAdd() const
{
   if (_slots.size() >= max_local_players || _client.getGameId() == -1)
   {
      return false;
   }

   return std::ranges::any_of(
      _controller_input.getDevices(), [this](const auto& device) { return !_controller_input.isAssigned(device.id); }
   );
}

void LocalPlayers::add()
{
   if (!canAdd())
   {
      return;
   }

   const auto devices = _controller_input.getDevices();
   const auto free = std::ranges::find_if(devices, [this](const auto& device) { return !_controller_input.isAssigned(device.id); });
   const ControllerInput::Id controller = free->id;

   Slot slot;
   slot.controller = controller;
   slot.client = std::make_unique<LocalPlayerClient>(_client.getHost(), nickForSlot(_slots.size()), _client.getGameId());
   slot.client->joinedSignal.connect([this](int32_t) { updateLocalPlayerIds(); });
   slot.client->joinFailedSignal.connect([this, controller]() { Timer::singleShot(0, [this, controller]() { remove(controller); }); });
   slot.client->rumbleSignal.connect([this, controller](float intensity, int32_t duration_ms)
                                     { _controller_input.rumble(controller, intensity, duration_ms); });

   _controller_input.setAssigned(controller, true);
   _slots.push_back(std::move(slot));
}

void LocalPlayers::remove(ControllerInput::Id controller)
{
   const auto it = std::ranges::find(_slots, controller, &Slot::controller);
   if (it == _slots.end())
   {
      return;
   }

   _controller_input.setAssigned(controller, false);
   _slots.erase(it);
   updateLocalPlayerIds();
}

void LocalPlayers::removeAll()
{
   for (const auto& slot : _slots)
   {
      _controller_input.setAssigned(slot.controller, false);
   }
   _slots.clear();
   updateLocalPlayerIds();
}

void LocalPlayers::update(bool in_game)
{
   for (const auto& slot : _slots)
   {
      slot.client->setKeys(in_game ? keysForButtons(_controller_input.getButtons(slot.controller)) : 0);
   }
}

size_t LocalPlayers::getCount() const
{
   return _slots.size();
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

std::string LocalPlayers::nickForSlot(size_t index) const
{
   const auto* login = GameSettings::getInstance()->getLoginSettings();
   const std::array<std::string, max_local_players> nicks{
      login->getPlayer2Nick(),
      login->getPlayer3Nick(),
      login->getPlayer4Nick(),
      login->getPlayer5Nick(),
      login->getPlayer6Nick(),
      login->getPlayer7Nick(),
      login->getPlayer8Nick(),
      login->getPlayer9Nick(),
      login->getPlayer10Nick(),
   };
   return nicks[std::min(index, nicks.size() - 1)];
}
