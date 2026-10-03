#include "helpmanager.h"

#include "tools/singleton.h"

HelpManager::HelpManager() = default;

HelpManager& HelpManager::getInstance()
{
   return Singleton<HelpManager>::Instance();
}

void HelpManager::addMessage(
   const std::string& page,
   const std::string& message,
   Constants::HelpSeverity severity,
   Constants::HelpLocation location,
   int delay
)
{
   messageAddedSignal(page, message, severity, location, delay);
}
