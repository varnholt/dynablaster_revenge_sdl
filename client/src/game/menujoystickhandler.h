#pragma once

#include "gamepadinput.h"
#include "menujoystickgraph.h"
#include "menujoystickmouseanimation.h"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>

class MenuDrawable;
class MenuMouseCursor;

/// \brief menu navigation with gamepads: one MenuJoystickGraph per page from
/// data/menus/menu_gamepad.ini, directions walk the graph, the bomb buttons click.
class MenuJoystickHandler
{
public:
   MenuJoystickHandler(MenuDrawable& menu, MenuMouseCursor& cursor, const GamepadInput& gamepad_input);

   /// \brief builds the graphs, the menu pages need to be loaded
   void initialize();

   void buttonPressed(GamepadInput::Button button);

   /// \brief a gamepad click that changed the page focusses the new page's default item
   void focusDefaultElement();

   /// \brief the real mouse took over
   void mouseMoved(int32_t x, int32_t y);
   void mousePressed();

   /// \brief drives the cursor animation
   void update();

private:
   MenuJoystickGraph* getCurrentGraph() const;
   void processKeyQueue();

   MenuDrawable& _menu;
   MenuMouseCursor& _cursor;
   const GamepadInput& _gamepad_input;
   MenuJoystickMouseAnimation _animation;
   std::map<std::string, std::unique_ptr<MenuJoystickGraph>> _graphs;
   std::deque<GamepadInput::Button> _key_queue;
   bool _gamepad_used = false;
};
