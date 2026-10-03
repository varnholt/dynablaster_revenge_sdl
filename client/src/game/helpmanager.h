#ifndef HELPMANAGER_H
#define HELPMANAGER_H

#include <string>

#include "constants.h"
#include "gamesignal.h"

class HelpManager
{
public:
   //! the instance is never destroyed
   static HelpManager& getInstance();

   HelpManager();

   // no live consumer ported yet - client/src/game/gamehelpdrawable.cpp (the toast HUD
   // that subscribed to this) hasn't been ported to client-sdl.
   Signal<const std::string&, const std::string&, Constants::HelpSeverity, Constants::HelpLocation, int> messageAddedSignal;

   void addMessage(
      const std::string& page,
      const std::string& message,
      Constants::HelpSeverity severity,
      Constants::HelpLocation location = Constants::HelpLocationTopRight,
      int delay = 0
   );
};

#endif  // HELPMANAGER_H
