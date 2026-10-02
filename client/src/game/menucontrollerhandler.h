#pragma once

#include "controllerinput.h"
#include "menucontrollercursoranimation.h"
#include "menucontrollergraph.h"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <string>

class MenuDrawable;
class MenuMouseCursor;

/// \brief menu navigation with controllers: one MenuControllerGraph per page from
/// data/menus/menu_controller.ini, directions walk the graph, the bomb buttons click.
class MenuControllerHandler
{
public:
   MenuControllerHandler(MenuDrawable& menu, MenuMouseCursor& cursor, const ControllerInput& controller_input);

   /// \brief builds the graphs, the menu pages need to be loaded
   void initialize();

   void buttonPressed(ControllerInput::Button button);

   /// \brief a controller click that changed the page focusses the new page's default item
   void focusDefaultItem();

   /// \brief the real mouse took over
   void mouseMoved(int32_t x, int32_t y);
   void mousePressed();

   /// \brief drives the cursor animation
   void update();

private:
   MenuControllerGraph* getCurrentGraph() const;
   void processButtonQueue();

   MenuDrawable& _menu;
   MenuMouseCursor& _cursor;
   const ControllerInput& _controller_input;
   MenuControllerCursorAnimation _animation;
   std::map<std::string, std::unique_ptr<MenuControllerGraph>> _graphs;
   std::deque<ControllerInput::Button> _button_queue;
   bool _controller_used = false;
};
