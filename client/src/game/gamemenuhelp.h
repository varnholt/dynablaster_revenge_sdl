#pragma once

#include <string>

/// \brief queues the menu pages' hints as help toasts (see GameHelpDrawable)
class GameMenuHelp
{
public:
   void pageChanged(const std::string& page);
};
