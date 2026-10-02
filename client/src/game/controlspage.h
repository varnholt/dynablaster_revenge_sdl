#pragma once

#include "controllerinput.h"
#include "controlssetup.h"
#include "localplayers.h"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

class BombermanClient;
class MenuDrawable;
class MenuPage;

/// \brief the page between choosing a game and joining it (dstar's controls.psd): one column per
/// player on this machine with its device, color and name. on OK the first playing column joins
/// as the main player and every other one as a LocalPlayers player.
///
/// mouse: the upper arrows pick the color, the lower ones the device. a controller steers its
/// own column: left/right pick the color, up/down move it to the neighbouring column, a bomb
/// button or start confirms. the keyboard does the same for the keyboard's column, return
/// confirms and escape cancels.
class ControlsPage
{
public:
   ControlsPage(MenuDrawable& menu, ControllerInput& controller_input, LocalPlayers& local_players, BombermanClient& client);
   ControlsPage(const ControlsPage&) = delete;
   ControlsPage& operator=(const ControlsPage&) = delete;
   ~ControlsPage();

   /// \brief shows the page; with the keyboard alone it's a single column to pick the color
   bool open(int32_t game_id, const std::string& return_page);

   bool isCurrentPage() const;

   const ControlsSetup& getSetup() const;

   void onActionRequest(const std::string& page, const std::string& action);
   void onControllerButtonPressed(ControllerInput::Id id, ControllerInput::Button button);

   /// \brief keyboard input while the page is shown, true if it was used
   bool onKeyPressed(SDL_Keycode key);

private:
   MenuPage* getPage() const;
   std::vector<std::string> getDefaultNames() const;
   void readNames();
   void refresh();
   void identify(size_t column);
   void confirm();
   void cancel();
   void joinLocalPlayers(bool joined);

   MenuDrawable& _menu;
   ControllerInput& _controller_input;
   LocalPlayers& _local_players;
   BombermanClient& _client;

   ControlsSetup _setup;
   int32_t _game_id = -1;
   std::string _return_page;
   int32_t _column_left = 0;  // where the PSD has the first column

   // joined once the main player is in the game
   std::vector<LocalPlayers::Player> _pending_players;
   std::vector<ControllerInput::Id> _pending_excluded;
   Signal<bool>::Connection _join_connection = 0;
   bool _join_connected = false;
};
