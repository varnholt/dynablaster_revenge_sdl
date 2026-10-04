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

   // shown by GameHelpDrawable
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
