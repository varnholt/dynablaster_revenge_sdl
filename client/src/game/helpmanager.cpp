#include "helpmanager.h"

HelpManager* HelpManager::sInstance = nullptr;


HelpManager::HelpManager()
{
}


HelpManager* HelpManager::getInstance()
{
   if (!sInstance)
   {
      sInstance = new HelpManager();
   }

   return sInstance;
}


void HelpManager::addMessage(
   const std::string& page,
   const std::string& message,
   Constants::HelpSeverity severity,
   Constants::HelpLocation location,
   int delay
)
{
   messageAddedSignal(
      page,
      message,
      severity,
      location,
      delay
   );
}
