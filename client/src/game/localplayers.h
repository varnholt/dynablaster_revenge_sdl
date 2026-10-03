#pragma once

#include "constants.h"
#include "controllerinput.h"
#include "localplayerclient.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

class BombermanClient;

/// \brief the players on this machine beyond the main one, each driven by its own controller or
/// the keyboard. controllers not taken here steer the main player unless they're excluded.
class LocalPlayers
{
public:
   //! the server's maximum is 10 players, the main one included
   static constexpr size_t max_local_players = 9;

   struct Player
   {
      std::optional<ControllerInput::Id> controller;  //!< none: the keyboard
      std::optional<Constants::Color> color;
      std::string nick;
   };

   LocalPlayers(ControllerInput& controller_input, BombermanClient& client);
   LocalPlayers(const LocalPlayers&) = delete;
   LocalPlayers& operator=(const LocalPlayers&) = delete;
   ~LocalPlayers();

   /// \brief the player joins the main player's game
   void add(const Player& player);

   /// \brief keeps a controller from steering the main player, until removeAll()
   void exclude(ControllerInput::Id controller);

   /// \brief all players leave, all controllers steer the main player again
   void removeAll();

   /// \brief forwards every assigned device's state to its player
   void update(bool in_game);

   size_t getCount() const;

   /// \brief some players are still connecting or joining
   bool isJoining() const;

   /// \brief the keyboard steers one of these players, not the main one
   bool isKeyboardAssigned() const;

private:
   struct Slot
   {
      uint32_t id = 0;
      std::optional<ControllerInput::Id> controller;
      std::unique_ptr<LocalPlayerClient> client;
   };

   void remove(uint32_t slot_id);
   void removeController(ControllerInput::Id controller);
   void updateLocalPlayerIds();
   uint8_t readKeyboard() const;

   ControllerInput& _controller_input;
   BombermanClient& _client;
   std::vector<Slot> _slots;
   uint32_t _next_slot_id = 0;
   std::unordered_set<ControllerInput::Id> _excluded;
   Signal<ControllerInput::Id>::Connection _device_removed_connection = 0;
};
