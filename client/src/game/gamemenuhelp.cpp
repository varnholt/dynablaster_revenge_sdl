#include "gamemenuhelp.h"

#include "bombermanclient.h"
#include "gamehelptexts.h"
#include "helpmanager.h"

#include <initializer_list>

namespace
{
// hints show after 15s on the same page
constexpr int HINT_DELAY = 15000;

void addHints(const std::string& page, std::initializer_list<const char*> hints)
{
   for (const char* hint : hints)
   {
      HelpManager::getInstance().addMessage(page, hint, Constants::HelpSeverityNotification, Constants::HelpLocationTopRight, HINT_DELAY);
   }
}
}  // namespace

void GameMenuHelp::pageChanged(const std::string& page)
{
   if (page == "data/menus/mainmenu.psd")
   {
      addHints(page, {TEXT_MAIN_SINGLE, TEXT_MAIN_MULTI, TEXT_MAIN_SERVER});
   }
   else if (page == "data/menus/selectgame.psd")
   {
      addHints(page, {TEXT_SELECT_GAME_SELECT, TEXT_SELECT_GAME_CREATE});
   }
   else if (page == "data/menus/creategame.psd")
   {
      addHints(page, {TEXT_CREATE_GAME_DEFINE});
   }
   else if (page == "data/menus/lounge.psd")
   {
      if (BombermanClient::getInstance().isPlayerOwner())
      {
         addHints(page, {TEXT_LOUNGE_START_GAME});
      }
      addHints(page, {TEXT_LOUNGE_CHAT});
   }
   else if (page == "data/menus/options_controls.psd")
   {
      addHints(page, {TEXT_OPTIONS_CONTROLLER, TEXT_OPTIONS_KEYBOARD});
   }
}
