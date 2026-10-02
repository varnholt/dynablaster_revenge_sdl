#include "menujoystickhandler.h"

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
constexpr const char* graph_file = "data/menus/menu_gamepad.ini";

// the key queue holds at most this many moves while the cursor is still gliding
constexpr size_t max_queued_keys = 5;

std::string unquoted(const std::string& text)
{
   return text.size() >= 2 && text.front() == '"' && text.back() == '"' ? text.substr(1, text.size() - 2) : text;
}

bool isMenuStopped()
{
   return GameStateMachine::getInstance()->getState() == Constants::GameStopped;
}
}  // namespace

MenuJoystickHandler::MenuJoystickHandler(MenuDrawable& menu, MenuMouseCursor& cursor, const GamepadInput& gamepad_input)
    : _menu(menu), _cursor(cursor), _gamepad_input(gamepad_input)
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
         if (!_key_queue.empty())
         {
            processKeyQueue();
         }
      }
   );
}

void MenuJoystickHandler::initialize()
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

      auto graph = std::make_unique<MenuJoystickGraph>(_animation);
      graph->mousePressSignal.connect(
         [this](int32_t x, int32_t y)
         {
            _gamepad_used = true;
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
            auto element = std::make_unique<MenuJoystickGraph::Element>();
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

MenuJoystickGraph* MenuJoystickHandler::getCurrentGraph() const
{
   MenuPage* page = Menu::getInstance()->getCurrentPage();
   if (!page)
   {
      return nullptr;
   }

   const auto it = _graphs.find(page->getFilename());
   return it != _graphs.end() ? it->second.get() : nullptr;
}

void MenuJoystickHandler::buttonPressed(GamepadInput::Button button)
{
   if (!isMenuStopped())
   {
      return;
   }

   switch (button)
   {
      case GamepadInput::ButtonUp:
      case GamepadInput::ButtonDown:
      case GamepadInput::ButtonLeft:
      case GamepadInput::ButtonRight:
      case GamepadInput::ButtonBomb:
         break;
      default:
         return;
   }

   if (_key_queue.size() < max_queued_keys)
   {
      _key_queue.push_back(button);
      if (!_animation.isBusy())
      {
         processKeyQueue();
      }
   }
}

void MenuJoystickHandler::processKeyQueue()
{
   const GamepadInput::Button button = _key_queue.front();
   _key_queue.pop_front();

   MenuJoystickGraph* graph = getCurrentGraph();
   if (!graph)
   {
      return;
   }

   switch (button)
   {
      case GamepadInput::ButtonUp:
         graph->walk(MenuJoystickGraph::Direction::North);
         break;
      case GamepadInput::ButtonDown:
         graph->walk(MenuJoystickGraph::Direction::South);
         break;
      case GamepadInput::ButtonLeft:
         graph->walk(MenuJoystickGraph::Direction::West);
         break;
      case GamepadInput::ButtonRight:
         graph->walk(MenuJoystickGraph::Direction::East);
         break;
      case GamepadInput::ButtonBomb:
         graph->button();
         break;
      default:
         break;
   }
}

void MenuJoystickHandler::focusDefaultElement()
{
   if (_gamepad_input.getDevices().empty() || !_gamepad_used || !isMenuStopped())
   {
      return;
   }

   if (MenuJoystickGraph* graph = getCurrentGraph())
   {
      graph->changeFocus(nullptr, graph->getDefaultPageItem());
   }
}

void MenuJoystickHandler::mouseMoved(int32_t x, int32_t y)
{
   _animation.setPosition(x, y);
}

void MenuJoystickHandler::mousePressed()
{
   _gamepad_used = false;
}

void MenuJoystickHandler::update()
{
   _animation.update(SDL_GetTicks());
}
