#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

/// \brief plays a development script given by the DYNABLASTER_DEV_SCRIPT environment variable:
/// lets a test run click through the menu, press keys and take screenshots while the window
/// stays in the background. One command per line:
///    wait <ms> | click <x> <y> (1920x1080 page space) | key <name> <ms> | down <name> | up <name>
///    shot <png path> | quit
class DevScript
{
public:
   DevScript();

   [[nodiscard]] bool isActive() const;

   //! runs the commands that are due
   void update(uint64_t now_ms);

   //! takes a pending screenshot of the frame just drawn
   void frameDrawn(int32_t width, int32_t height);

   std::function<void(int32_t, int32_t)> _click;
   std::function<void()> _quit;

private:
   struct Command
   {
      std::string _name;
      std::vector<std::string> _arguments;
   };

   std::vector<Command> _commands;
   size_t _next = 0;
   uint64_t _resume_at = 0;
   std::string _pending_shot;

   struct HeldKey
   {
      uint32_t _key = 0;
      uint64_t _release_at = 0;
   };

   std::vector<HeldKey> _held;
};
