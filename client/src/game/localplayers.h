#pragma once

#include "controllerinput.h"
#include "localplayerclient.h"

#include <cstdint>
#include <memory>
#include <vector>

class BombermanClient;

/// \brief the players on this machine beyond the main one, each driven by its own controller.
/// controllers not assigned here (and the keyboard) steer the main player.
class LocalPlayers
{
public:
   //! the server's maximum is 10 players, the main one included
   static constexpr size_t max_local_players = 9;

   LocalPlayers(ControllerInput& controller_input, BombermanClient& client);
   LocalPlayers(const LocalPlayers&) = delete;
   LocalPlayers& operator=(const LocalPlayers&) = delete;
   ~LocalPlayers();

   /// \brief there is a free controller and the main player is in a game
   bool canAdd() const;

   /// \brief the next free controller joins the main player's game as a new player
   void add();

   void removeAll();

   /// \brief forwards every assigned controller's state to its player
   void update(bool in_game);

   size_t getCount() const;

private:
   struct Slot
   {
      ControllerInput::Id controller = 0;
      std::unique_ptr<LocalPlayerClient> client;
   };

   void remove(ControllerInput::Id controller);
   void updateLocalPlayerIds();
   std::string nickForSlot(size_t index) const;

   ControllerInput& _controller_input;
   BombermanClient& _client;
   std::vector<Slot> _slots;
   Signal<ControllerInput::Id>::Connection _device_removed_connection = 0;
};
