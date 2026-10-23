#include "devscript.h"

#include "screenshot.h"

#include "logging.h"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <fstream>
#include <sstream>

namespace
{
void pushKey(uint32_t key, bool down)
{
   SDL_Event event{};
   event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
   event.key.key = static_cast<SDL_Keycode>(key);
   event.key.down = down;
   SDL_PushEvent(&event);
}
}  // namespace

DevScript::DevScript()
{
   const char* path = std::getenv("DYNABLASTER_DEV_SCRIPT");

   if (!path)
   {
      return;
   }

   std::ifstream file(path);
   std::string line;

   while (std::getline(file, line))
   {
      std::istringstream words(line);
      Command command;
      words >> command._name;

      if (command._name.empty() || command._name[0] == '#')
      {
         continue;
      }

      std::string argument;
      while (words >> argument)
      {
         command._arguments.push_back(argument);
      }

      _commands.push_back(std::move(command));
   }

   qDebug("DevScript: %zu commands from %s", _commands.size(), path);
}

bool DevScript::isActive() const
{
   return !_commands.empty();
}

void DevScript::update(uint64_t now_ms)
{
   for (auto it = _held.begin(); it != _held.end();)
   {
      if (now_ms >= it->_release_at)
      {
         pushKey(it->_key, false);
         it = _held.erase(it);
      }
      else
      {
         ++it;
      }
   }

   while (_next < _commands.size() && now_ms >= _resume_at && _pending_shot.empty())
   {
      const Command& command = _commands[_next++];
      const auto argument = [&command](size_t index) { return index < command._arguments.size() ? command._arguments[index] : std::string(); };

      if (command._name == "wait")
      {
         _resume_at = now_ms + std::stoull(argument(0));
      }
      else if (command._name == "click" && _click)
      {
         _click(std::stoi(argument(0)), std::stoi(argument(1)));
      }
      else if (command._name == "key" || command._name == "down" || command._name == "up")
      {
         const SDL_Keycode key = SDL_GetKeyFromName(argument(0).c_str());

         if (command._name != "up")
         {
            pushKey(key, true);
         }

         if (command._name == "key")
         {
            _held.push_back({key, now_ms + std::stoull(argument(1))});
         }
         else if (command._name == "up")
         {
            pushKey(key, false);
         }
      }
      else if (command._name == "shot")
      {
         _pending_shot = argument(0);
      }
      else if (command._name == "quit" && _quit)
      {
         _quit();
      }
   }
}

void DevScript::frameDrawn(int32_t width, int32_t height)
{
   if (!_pending_shot.empty())
   {
      saveScreenshot(_pending_shot, width, height);
      qDebug("DevScript: screenshot %s", _pending_shot.c_str());
      _pending_shot.clear();
   }
}
