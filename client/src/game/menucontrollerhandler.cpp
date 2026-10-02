#include "menucontrollerhandler.h"

#include "constants.h"
#include "game/gamestatemachine.h"
#include "menus/menu.h"
#include "menus/menudrawable.h"
#include "menus/menumousecursor.h"
#include "menus/menupage.h"
#include "settings.h"

#include <SDL3/SDL.h>

namespace
{
constexpr const char* graph_file = "data/menus/menu_controller.ini";

// at most this many moves are queued while the cursor is still gliding
constexpr size_t max_queued_buttons = 5;

std::string unquoted(const std::string& text)
{
   return text.size() >= 2 && text.front() == '"' && text.back() == '"' ? text.substr(1, text.size() - 2) : text;
}

bool isInMenu()
{
   return GameStateMachine::getInstance()->getState() == Constants::GameStopped;
}
}  // namespace

MenuControllerHandler::MenuControllerHandler(MenuDrawable& menu, MenuMouseCursor& cursor, const ControllerInput& controller_input)
    : _menu(menu), _cursor(cursor), _controller_input(controller_input)
{
   _animation.movedSignal.connect(
      [this](int32_t x, int32_t y)
      {
         _menu.mouseMoveEvent(x, y);
         _cursor.mouseMoveEvent(x, y);
      }
   );

   // moves pressed while the cursor was gliding are processed afterwards
   _animation.doneSignal.connect(
      [this]()
      {
         if (!_button_queue.empty())
         {
            processButtonQueue();
         }
      }
   );
}

void MenuControllerHandler::initialize()
{
   Settings settings(graph_file);

   settings.beginGroup("pages");
   const auto page_keys = settings.childKeys();
   settings.endGroup();

   for (const auto& page_key : page_keys)
   {
      const std::string psd_name = settings.value("pages/" + page_key).toString();
      MenuPage* page = Menu::getInstance()->getPageByName(psd_name);
      if (!page)
      {
         continue;
      }

      auto graph = std::make_unique<MenuControllerGraph>(_animation);
      graph->clickSignal.connect(
         [this](int32_t x, int32_t y)
         {
            _controller_used = true;
            _menu.mousePressEvent(x, y);
            _cursor.mousePressEvent(x, y);
            _cursor.mouseReleaseEvent();
         }
      );

      settings.beginGroup(page_key);
      const auto element_names = settings.childKeys();
      settings.endGroup();

      for (const auto& element_name : element_names)
      {
         const auto items = settings.value(page_key + "/" + element_name).toStringList();

         // element=north,south,east,west
         if (items.size() == 4)
         {
            auto element = std::make_unique<MenuControllerGraph::Element>();
            element->item = page->getPageItem(element_name);
            element->north_item = page->getPageItem(unquoted(items[0]));
            element->south_item = page->getPageItem(unquoted(items[1]));
            element->east_item = page->getPageItem(unquoted(items[2]));
            element->west_item = page->getPageItem(unquoted(items[3]));

            if (!element->item)
            {
               SDL_Log("%s: %s has no item '%s'", graph_file, psd_name.c_str(), element_name.c_str());
               continue;
            }
            graph->add(std::move(element));
         }
         else if (items.size() == 1 && element_name == "default")
         {
            graph->setDefaultPageItem(page->getPageItem(unquoted(items.front())));
         }
      }

      graph->link();
      _graphs[psd_name] = std::move(graph);
   }
}

MenuControllerGraph* MenuControllerHandler::getCurrentGraph() const
{
   MenuPage* page = Menu::getInstance()->getCurrentPage();
   if (!page)
   {
      return nullptr;
   }

   const auto it = _graphs.find(page->getFilename());
   return it != _graphs.end() ? it->second.get() : nullptr;
}

void MenuControllerHandler::buttonPressed(ControllerInput::Button button)
{
   if (!isInMenu())
   {
      return;
   }

   switch (button)
   {
      case ControllerInput::ButtonUp:
      case ControllerInput::ButtonDown:
      case ControllerInput::ButtonLeft:
      case ControllerInput::ButtonRight:
      case ControllerInput::ButtonBomb:
         break;
      default:
         return;
   }

   if (_button_queue.size() < max_queued_buttons)
   {
      _button_queue.push_back(button);
      if (!_animation.isBusy())
      {
         processButtonQueue();
      }
   }
}

void MenuControllerHandler::processButtonQueue()
{
   const ControllerInput::Button button = _button_queue.front();
   _button_queue.pop_front();

   MenuControllerGraph* graph = getCurrentGraph();
   if (!graph)
   {
      return;
   }

   switch (button)
   {
      case ControllerInput::ButtonUp:
         graph->walk(MenuControllerGraph::Direction::North);
         break;
      case ControllerInput::ButtonDown:
         graph->walk(MenuControllerGraph::Direction::South);
         break;
      case ControllerInput::ButtonLeft:
         graph->walk(MenuControllerGraph::Direction::West);
         break;
      case ControllerInput::ButtonRight:
         graph->walk(MenuControllerGraph::Direction::East);
         break;
      case ControllerInput::ButtonBomb:
         graph->click();
         break;
      default:
         break;
   }
}

void MenuControllerHandler::focusDefaultItem()
{
   if (_controller_input.getDevices().empty() || !_controller_used || !isInMenu())
   {
      return;
   }

   if (MenuControllerGraph* graph = getCurrentGraph())
   {
      graph->changeFocus(nullptr, graph->getDefaultPageItem());
   }
}

void MenuControllerHandler::mouseMoved(int32_t x, int32_t y)
{
   _animation.setPosition(x, y);
}

void MenuControllerHandler::mousePressed()
{
   _controller_used = false;
}

void MenuControllerHandler::update()
{
   _animation.update(SDL_GetTicks());
}
