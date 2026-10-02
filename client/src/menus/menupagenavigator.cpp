#include "menupagenavigator.h"

#include "game/bombermanclient.h"
#include "game/gamesettings.h"
#include "game/gameversion.h"
#include "game/soundmanager.h"
#include "game/wordwrap.h"
#include "gameinformation.h"
#include "levels/level.h"

#include "hosthistory.h"
#include "menu.h"
#include "menupage.h"
#include "menupagecheckboxitem.h"
#include "menupagecomboboxitem.h"
#include "menupageeditablecomboboxitem.h"
#include "menupagelabelitem.h"
#include "menupagelistitem.h"
#include "menupagepixmapitem.h"
#include "menupageslideritem.h"
#include "menupagetextedit.h"

#include "playerinfo.h"
#include "stringutils.h"
#include "timer.h"

#include "logging.h"

#include <SDL3_net/SDL_net.h>

#include <algorithm>
#include <cstdlib>

namespace
{

// Mirrors client/src/menus/gamemenudefines.h's page/action string constants. Not #include-d
// directly - that header also defines networking-only action names this class deliberately
// doesn't handle, and pulling it in would imply more coverage than actually exists here.
const char* const kMainMenu = "data/menus/mainmenu.psd";
const char* const kMainMenuActionOptions = "button_options_active";
const char* const kMainMenuActionAbout = "button_about_active";
const char* const kMainMenuActionQuit = "button_quit_active";

const char* const kGameSelect = "data/menus/selectgame.psd";
const char* const kGameSelectActionCreate = "button_create_active";
const char* const kGameSelectActionBack = "button_back_active";

const char* const kGameCreate = "data/menus/creategame.psd";
const char* const kGameCreateActionCancel = "button_cancel_active";

const char* const kOptionsVideo = "data/menus/options_video.psd";
const char* const kOptionsAudio = "data/menus/options_audio.psd";
const char* const kOptionsControls = "data/menus/options_controls.psd";
const char* const kOptionsGame = "data/menus/options_game.psd";
const char* const kOptionsActionOk = "button_ok_active";
const char* const kOptionsActionCancel = "button_cancel_active";
const char* const kOptionsActionVideo = "button_video_active";
const char* const kOptionsActionAudio = "button_audio_active";
const char* const kOptionsActionControls = "button_controls_active";
const char* const kOptionsActionGame = "button_game_active";
const char* const kOptionsAudioActionRestoreDefaults = "button_default_active";
const char* const kOptionsAudioSliderMusic = "slider_music";
const char* const kOptionsAudioSliderSfx = "slider_game";

const char* const kAbout = "data/menus/about.psd";
const char* const kAboutActionBack = "button_back_active";
const char* const kLounge = "data/menus/lounge.psd";
const char* const kLoungeActionStart = "button_start_active";
const char* const kLoungeActionBack = "button_leave_active";
const char* const kLoungeAddPlayer = "button_addplayer";
const char* const kLoungeLineeditSay = "lineedit_say";
const char* const kLoungeTableMain = "table_lounge_main";

const char* const kMainMenuActionSingle = "button_single_active";
const char* const kMainMenuActionMulti = "button_multi_active";
const char* const kMainMenuActionPouet = "button_pouet_active";
const char* const kMainMenuActionFacebook = "button_facebook_active";
const char* const kMainMenuActionHome = "button_home_active";
const char* const kGameSelectActionJoin = "button_join_active";
const char* const kGameCreateActionOk = "button_ok_active";

bool isOptionsPage(const std::string& page)
{
   return page == kOptionsVideo || page == kOptionsAudio || page == kOptionsControls || page == kOptionsGame;
}

// matches GameMenuWorkflow::isHostLocal() - true if the configured host resolves to one of this
// machine's own network interfaces, in which case MULTI also hosts an in-process server (same as
// SINGLE always does), rather than only connecting out to a remote one.
bool isHostLocal(const std::string& host_name)
{
#ifdef __SWITCH__
   // libnx has no getifaddrs, so SDL_net cannot enumerate loopback here.
   if (host_name == "127.0.0.1" || StringUtils::toLower(host_name) == "localhost")
      return true;
#endif
   bool local = false;

   int count = 0;
   NET_Address** addresses = NET_GetLocalAddresses(&count);

   if (addresses)
   {
      for (int i = 0; i < count && !local; i++)
      {
         const char* address = NET_GetAddressString(addresses[i]);

         if (address && host_name == std::string(address))
         {
            local = true;
         }
      }

      NET_FreeLocalAddresses(addresses);
   }

   return local;
}

bool needsBrowser(const std::string& action)
{
   return action == kMainMenuActionPouet || action == kMainMenuActionFacebook || action == kMainMenuActionHome;
}

void logUnhandled(const std::string& page, const std::string& action)
{
   if (action.empty())
   {
      return;
   }

   if (needsBrowser(action))
   {
      qDebug("MenuPageNavigator: page=%s action=%s needs opening an external browser (not implemented)", page.c_str(), action.c_str());
   }
   // else: not a real menu action (e.g. an editablecombobox's own internal layer-name emission) -
   // the real GameMenuWorkflow doesn't log these either, so neither do we.
}

}  // namespace

MenuPageNavigator::MenuPageNavigator()
{
   // matches GameMenuInterfaceCreate's constructor.
   _sorted_level_names.push_back(Level::getLevelName(Level::LevelCastle));
   _sorted_level_names.push_back(Level::getLevelName(Level::LevelMansion));
   _sorted_level_names.push_back(Level::getLevelName(Level::LevelSpace));

   _sorted_level_dir_names.push_back(Level::getLevelDirectoryName(Level::LevelCastle));
   _sorted_level_dir_names.push_back(Level::getLevelDirectoryName(Level::LevelMansion));
   _sorted_level_dir_names.push_back(Level::getLevelDirectoryName(Level::LevelSpace));

   // BombermanClient must already be constructed+initialize()'d by main.cpp before this runs -
   // getInstance() doesn't self-construct (matches the real client/src/game/bombermanclientgui.cpp
   // construction order).
   BombermanClient* client = BombermanClient::getInstance();
   _login_response_connection = client->loginResponseSignal.connect([this](bool granted) { onLoginResponse(granted); });
   _create_game_response_connection = client->createGameResponseSignal.connect([this](bool granted, int game_id, bool owner)
                                                                               { onCreateGameResponse(granted, game_id, owner); });
   _join_game_response_connection = client->joinGameResponseSignal.connect([this](bool success) { onJoinGameResponse(success); });
   _game_started_connection = client->gameStartedSignal.connect([this]() { onGameStarted(); });

   // matches GameMenuInterfaceLounge's constructor connection - keeps the lounge's player rows
   // (nick/wins/rank/owner-icon) live-updated whenever the player set changes (join/leave/bot
   // added). Without this, bots that join after the lounge page is already showing never appear.
   _player_info_map_updated_connection =
      client->playerInfoMapUpdatedSignal.connect([this](std::map<int, PlayerInfo*>* info_map) { onPlayerInfoMapUpdated(info_map); });

   // matches GameMenuWorkflow's own connection to BombermanClient::messageReceived - lounge chat.
   _message_received_connection = client->messageReceivedSignal.connect([this](int sender_id, const std::string& message, bool finished)
                                                                        { onMessageReceived(sender_id, message, finished); });

   // matches MenuWorkflow::initialize() calling deserializeLoginData() once at startup - the
   // main menu is already the current page by construction time (no pageChanged() fires for it
   // the very first time), so this can't wait for onPageChanged()'s own kMainMenu branch below.
   deserializeLoginData();
   deserializeVersion();
}

MenuPageNavigator::~MenuPageNavigator()
{
   // only still connected while GAME_CREATE / OPTIONS_AUDIO is the current page
   setMonitorCreateGameOptionsEnabled(false);
   setMonitorAudioSettingsEnabled(false);

   if (BombermanClient* client = BombermanClient::getInstance())
   {
      client->loginResponseSignal.disconnect(_login_response_connection);
      client->createGameResponseSignal.disconnect(_create_game_response_connection);
      client->joinGameResponseSignal.disconnect(_join_game_response_connection);
      client->gameStartedSignal.disconnect(_game_started_connection);
      client->playerInfoMapUpdatedSignal.disconnect(_player_info_map_updated_connection);
      client->messageReceivedSignal.disconnect(_message_received_connection);
   }
}

void MenuPageNavigator::onActionRequest(const std::string& page, const std::string& action)
{
   if (page == kMainMenu)
   {
      // matches GameMenuWorkflow's "extract login data from menu" calls before SINGLE/MULTI/
      // OPTIONS/ABOUT - captures whatever's currently in the nick/host fields into GameSettings
      // + HostHistory. Harmless to run for every mainmenu action (facebook/pouet/home/quit too).
      updateLoginData();

      if (action == kMainMenuActionOptions)
      {
         pageChangeRequestSignal(kOptionsVideo);
      }
      else if (action == kMainMenuActionAbout)
      {
         pageChangeRequestSignal(kAbout);
      }
      else if (action == kMainMenuActionQuit)
      {
         quitRequestSignal();
      }
      else if (action == kMainMenuActionSingle)
      {
         // matches GameMenuWorkflow's MAINMENU_ACTION_SINGLE handler exactly: single player
         // always hosts an in-process server and logs into it over loopback.
         BombermanClient::getInstance()->setGameMode(Constants::GameModeSinglePlayer);
         BombermanClient::getInstance()->host();
         BombermanClient::getInstance()->loginRequest("127.0.0.1", GameSettings::getInstance()->getLoginSettings()->getNick());
      }
      else if (action == kMainMenuActionMulti)
      {
         // matches GameMenuWorkflow's MAINMENU_ACTION_MULTI handler: only hosts if the
         // configured host is actually this machine, always logs in to whatever host is set.
         BombermanClient::getInstance()->setGameMode(Constants::GameModeMultiPlayer);

         const std::string host = GameSettings::getInstance()->getLoginSettings()->getHost();

         if (isHostLocal(host))
         {
            BombermanClient::getInstance()->host();
         }

         BombermanClient::getInstance()->loginRequest(host, GameSettings::getInstance()->getLoginSettings()->getNick());
      }
      else
      {
         logUnhandled(page, action);
      }
   }
   else if (page == kGameSelect)
   {
      if (action == kGameSelectActionCreate)
      {
         pageChangeRequestSignal(kGameCreate);
      }
      else if (action == kGameSelectActionBack)
      {
         pageChangeRequestSignal(kMainMenu);
      }
      else if (action == kGameSelectActionJoin)
      {
         // the real GAME_SELECT_ACTION_JOIN reads the operator-selected row out of the game
         // table (GameMenuInterfaceSelect::getSelectedGame()) - that table selection isn't wired
         // in this port yet, so this joins the first known game instead. Real enough to prove the
         // join->lounge chain; picking a specific game is a later refinement.
         const std::vector<GameInformation>* games = BombermanClient::getInstance()->getGames();
         if (games && !games->empty())
         {
            requestJoin(games->front().getId(), kGameSelect);
         }
         else
         {
            qDebug("MenuPageNavigator: JOIN clicked with no games known yet");
         }
      }
      else
      {
         logUnhandled(page, action);
      }
   }
   else if (page == kGameCreate)
   {
      if (action == kGameCreateActionCancel)
      {
         // the real GameMenuWorkflow picks GAME_SELECT vs MAINMENU here based on
         // BombermanClient::getGameMode() (single- vs multiplayer) - that state doesn't exist in
         // this port yet, so this always goes back to GAME_SELECT, the multiplayer flow's own
         // back target and the only way to have reached GAME_CREATE at all right now.
         pageChangeRequestSignal(kGameSelect);
      }
      else if (action == kGameCreateActionOk)
      {
         createGame();
      }
      else
      {
         logUnhandled(page, action);
      }
   }
   else if (isOptionsPage(page))
   {
      if (action == kOptionsActionOk || action == kOptionsActionCancel)
      {
         // audio: matches GameMenuInterfaceOptions::storeOptions()/restoreAudioOptions() - OK
         // persists the (already live-applied) SoundManager volume to disk, Cancel reverts
         // SoundManager back to whatever was last persisted, discarding an unsaved drag.
         // video/controls/game options aren't backed by live-applied state yet, so they stay
         // a plain navigate-back.
         if (page == kOptionsAudio)
         {
            GameSettings::AudioSettings* audio_settings = GameSettings::getInstance()->getAudioSettings();

            if (action == kOptionsActionOk)
            {
               audio_settings->setVolumeMusic(SoundManager::getInstance()->getVolumeMusic());
               audio_settings->setVolumeSfx(SoundManager::getInstance()->getVolumeSfx());
               audio_settings->serialize();
            }
            else
            {
               SoundManager::getInstance()->setVolumeMusic(audio_settings->getVolumeMusic());
               SoundManager::getInstance()->setVolumeSfx(audio_settings->getVolumeSfx());
            }
         }

         pageChangeRequestSignal(kMainMenu);
      }
      else if (action == kOptionsActionVideo)
      {
         pageChangeRequestSignal(kOptionsVideo);
      }
      else if (action == kOptionsActionAudio)
      {
         pageChangeRequestSignal(kOptionsAudio);
      }
      else if (action == kOptionsActionControls)
      {
         pageChangeRequestSignal(kOptionsControls);
      }
      else if (action == kOptionsActionGame)
      {
         pageChangeRequestSignal(kOptionsGame);
      }
      else if (page == kOptionsAudio && action == kOptionsAudioActionRestoreDefaults)
      {
         restoreAudioDefaults();
      }
      else
      {
         logUnhandled(page, action);
      }
   }
   else if (page == kAbout)
   {
      if (action == kAboutActionBack)
      {
         pageChangeRequestSignal(kMainMenu);
      }
      else
      {
         logUnhandled(page, action);
      }
   }
   else if (page == kLounge)
   {
      if (action == kLoungeActionStart)
      {
         BombermanClient::getInstance()->startGame(BombermanClient::getInstance()->getGameId());
      }
      else if (action == kLoungeActionBack)
      {
         BombermanClient::getInstance()->leaveGameRequest();
         pageChangeRequestSignal(kGameSelect);
      }
      else if (action == kLoungeLineeditSay)
      {
         // matches GameMenuWorkflow::onActionRequest()'s LOUNGE_LINEEDIT_SAY branch - MenuPage::
         // keyPressed() (menupage.cpp) only emits this actionRequest on Return/Enter, so this is
         // the "finished typing, send it" path. Per-keystroke "still typing" notifications would
         // need MenuPage::actionKeyPressed() wired up too - not done here, matches the existing
         // "typing bubble never shown" simplification (see updateLoungePlayerList()).
         MenuPage* lounge_page = Menu::getInstance()->getPageByName(kLounge);
         auto* say_item = dynamic_cast<MenuPageTextEditItem*>(lounge_page->getPageItem(kLoungeLineeditSay));

         if (say_item)
         {
            const std::string message = say_item->getText();

            if (!StringUtils::trim(message).empty())
            {
               // lounge chat always broadcasts to everyone (receiverId -1)
               BombermanClient::getInstance()->sendMessage(message, true);
               say_item->setText("");
            }
         }
      }
      else
      {
         logUnhandled(page, action);
      }
   }
   else
   {
      logUnhandled(page, action);
   }
}

void MenuPageNavigator::onLoginResponse(bool granted)
{
   // matches GameMenuWorkflow::loginResponse(): single player goes straight to GAME_CREATE
   // (create-and-join is automatic from there), multiplayer goes to GAME_SELECT to pick/create.
   if (granted)
   {
      const Constants::GameMode mode = BombermanClient::getInstance()->getGameMode();
      pageChangeRequestSignal(mode == Constants::GameModeMultiPlayer ? kGameSelect : kGameCreate);
   }
   else
   {
      qDebug("MenuPageNavigator: login denied");
   }
}

void MenuPageNavigator::onCreateGameResponse(bool granted, int game_id, bool owner)
{
   // matches GameMenuWorkflow::createGameResponse(): the creator auto-joins the game they just
   // made; everyone else (broadcast of the same response) just sees the updated game list.
   if (granted && owner)
   {
      requestJoin(game_id, kGameCreate);
   }
   else if (granted)
   {
      pageChangeRequestSignal(kGameSelect);
   }
}

void MenuPageNavigator::onJoinGameResponse(bool success)
{
   if (success)
   {
      pageChangeRequestSignal(kLounge);

      // matches GameMenuWorkflow::pageChanged()'s LOUNGE branch (real GameMenuWorkflow isn't
      // ported - this is the only trigger for BombermanClient::initializeBots(), which was
      // otherwise fully wired to BotFactory but never called from anywhere in this port).
      // after the players of this machine, so they get their colors first and bots the rest
      Timer::singleShot(1000, [this]() { initializeBots(20); });
   }
}

void MenuPageNavigator::onGameStarted()
{
   // real gameplay handoff (level loading, HUD, in-game rendering/input) is the separate,
   // not-yet-scoped "Phase 5" - this proves the network state machine reaches GameActive for
   // real, nothing more.
   qDebug("MenuPageNavigator: gameStarted() - real gameplay handoff not implemented yet (Phase 5)");
}

void MenuPageNavigator::onPageChanged(const std::string& page)
{
   // matches GameMenuWorkflow::pageChanged(): monitoring is disabled unconditionally first, then
   // re-enabled only for the page actually being shown.
   setMonitorCreateGameOptionsEnabled(false);
   setMonitorAudioSettingsEnabled(false);

   if (page == kMainMenu)
   {
      // matches GameMenuWorkflow::pageChanged()'s MAINMENU branch calling
      // mGameMenuInterfaceMain->deserializeLoginData() - repopulates the host combobox/nick
      // field from the saved settings + host history every time the main menu is (re-)shown,
      // not just on first app startup.
      deserializeLoginData();
      deserializeVersion();
   }
   else if (page == kGameCreate)
   {
      deserializeCreateGameData();
      initializeCreateGameOptions();
      setMonitorCreateGameOptionsEnabled(true);
   }
   else if (page == kLounge)
   {
      // matches GameMenuWorkflow::pageChanged()'s LOUNGE branch calling
      // mGameMenuInterfaceLounge->playerInfoMapUpdated(...) directly once, on top of the live
      // signal connection - populates the rows immediately instead of waiting for the next
      // join/leave to trigger a redraw.
      updateLoungePlayerList(BombermanClient::getInstance()->getPlayerInfoMap());

      // players on this machine are set up on the controls page before joining
      if (MenuPageItem* add_player = Menu::getInstance()->getPageByName(kLounge)->getPageItem(kLoungeAddPlayer))
      {
         add_player->setVisible(false);
      }
   }
   else if (page == kOptionsAudio)
   {
      deserializeAudioSettings();
      setMonitorAudioSettingsEnabled(true);
   }
}

void MenuPageNavigator::setJoinHandler(JoinHandler handler)
{
   _join_handler = std::move(handler);
}

void MenuPageNavigator::setBotsWaitCondition(std::function<bool()> condition)
{
   _bots_wait_condition = std::move(condition);
}

void MenuPageNavigator::initializeBots(int32_t remaining_tries)
{
   if (remaining_tries > 0 && _bots_wait_condition && _bots_wait_condition())
   {
      Timer::singleShot(250, [this, remaining_tries]() { initializeBots(remaining_tries - 1); });
      return;
   }

   BombermanClient::getInstance()->initializeBots();
}

void MenuPageNavigator::requestJoin(int game_id, const std::string& return_page)
{
   if (!_join_handler || !_join_handler(game_id, return_page))
   {
      BombermanClient::getInstance()->joinGame(game_id);
   }
}

void MenuPageNavigator::onPlayerInfoMapUpdated(std::map<int, PlayerInfo*>* player_info)
{
   updateLoungePlayerList(player_info);
}

void MenuPageNavigator::updateLoungePlayerList(std::map<int, PlayerInfo*>* player_info)
{
   // matches GameMenuInterfaceLounge::playerInfoMapUpdated() - only touches the UI while the
   // lounge page is actually the one showing (mirrors the original's own current_page == page
   // guard, since this can also fire while some other page, e.g. main menu after leaving, is up).
   MenuPage* current_page = Menu::getInstance()->getCurrentPage();
   MenuPage* page = Menu::getInstance()->getPageByName(kLounge);

   if (current_page != page || !player_info)
   {
      return;
   }

   _player_id_to_index_map.clear();

   struct ScoreEntry
   {
      PlayerInfo* player;
      int score;
   };

   std::vector<ScoreEntry> score_list;
   for (const auto& [id, info] : *player_info)
   {
      score_list.push_back({info, static_cast<int>(info->getOverallStats().getWins())});
   }

   std::sort(score_list.begin(), score_list.end(), [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });

   // initially hide all rows
   for (int i = 1; i <= 10; i++)
   {
      auto* nick_item = dynamic_cast<MenuPageLabelItem*>(current_page->getPageItem("label_p" + std::to_string(i)));
      auto* active_box_item = current_page->getPageItem("p" + std::to_string(i) + "_box_active");
      auto* owner_item = current_page->getPageItem("p" + std::to_string(i) + "_leader_icon");
      auto* player_item = current_page->getPageItem("p" + std::to_string(i) + "_icon");
      auto* rank_item = current_page->getPageItem("label_rank_" + std::to_string(i));
      auto* wins_item = dynamic_cast<MenuPageLabelItem*>(current_page->getPageItem("label_p" + std::to_string(i) + "_wins"));
      // matches GameMenuInterfaceLounge::initializeLoungeStates() - only ever shown by the
      // typing-indicator feature (updatePlayerTyping()/removePlayerTyping()), which isn't ported
      // (see the deferred chat handling below) - so it must be force-hidden here instead, or the
      // PSD's raw default visibility leaks through unmodified for every row.
      auto* typing_box_item = current_page->getPageItem("p" + std::to_string(i) + "_box_type");

      if (nick_item)
      {
         nick_item->setText("");
      }
      if (active_box_item)
      {
         active_box_item->setVisible(false);
      }
      if (owner_item)
      {
         owner_item->setVisible(false);
      }
      if (player_item)
      {
         player_item->setVisible(false);
      }
      if (rank_item)
      {
         rank_item->setVisible(false);
      }
      if (wins_item)
      {
         wins_item->setVisible(false);
      }
      if (typing_box_item)
      {
         typing_box_item->setVisible(false);
      }
   }

   int counter = 0;
   for (const ScoreEntry& entry : score_list)
   {
      counter++;

      PlayerInfo* player = entry.player;
      const int color = static_cast<int>(player->getColor());
      GameInformation* game_info = BombermanClient::getInstance()->getCurrentGameInformation();
      const bool owner = game_info && (player->getId() == game_info->getCreatorId());

      auto* nick_item = dynamic_cast<MenuPageLabelItem*>(current_page->getPageItem("label_p" + std::to_string(counter)));
      auto* wins_item = dynamic_cast<MenuPageLabelItem*>(current_page->getPageItem("label_p" + std::to_string(counter) + "_wins"));
      auto* rank_item = current_page->getPageItem("label_rank_" + std::to_string(counter));
      auto* active_box_item = current_page->getPageItem("p" + std::to_string(counter) + "_box_active");
      auto* owner_item = current_page->getPageItem("p" + std::to_string(counter) + "_leader_icon");
      auto* player_item = current_page->getPageItem("p" + std::to_string(color) + "_icon");

      // note: the original also nudges player_item's active-layer Y to align with active_box_item's
      // top (player_item->getCurrentLayer()->setY(...)) - PSDLayer::setY() isn't ported in this
      // port (only getters + setOpacity exist), so that pixel-alignment tweak is skipped; the row
      // still shows correctly, just not pixel-perfect vertically.

      _player_id_to_index_map[player->getId()] = counter;

      if (wins_item)
      {
         wins_item->setText(std::to_string(entry.score));
         wins_item->setColor(Color(counter <= 3 ? "#fbfe00" : "#3b7d9d"));
         wins_item->setVisible(true);
      }

      if (nick_item)
      {
         nick_item->setText(player->getNick());
      }
      if (active_box_item)
      {
         active_box_item->setVisible(true);
      }
      if (owner_item)
      {
         owner_item->setVisible(owner);
      }
      if (player_item)
      {
         player_item->setVisible(true);
      }
      if (rank_item)
      {
         rank_item->setVisible(true);
      }
   }
}

void MenuPageNavigator::onMessageReceived(int sender_id, const std::string& message, bool finished)
{
   // matches GameMenuWorkflow::messageReceived() - typing-in-progress notifications
   // (finished == false) would drive the "typing bubble" via updatePlayerTyping(), which isn't
   // ported (see the comment in updateLoungePlayerList()); only finished messages are displayed.
   if (finished)
   {
      addLoungeMessage(sender_id, message);
   }
}

void MenuPageNavigator::addLoungeMessage(int sender_id, const std::string& message)
{
   // matches GameMenuInterfaceLounge::addLoungeMessage() - only touches the UI while the lounge
   // page is actually showing.
   MenuPage* lounge_page = Menu::getInstance()->getPageByName(kLounge);
   MenuPage* current_page = Menu::getInstance()->getCurrentPage();

   if (lounge_page != current_page)
   {
      return;
   }

   auto* say_item = dynamic_cast<MenuPageTextEditItem*>(lounge_page->getPageItem(kLoungeLineeditSay));
   auto* table_item = dynamic_cast<MenuPageListItem*>(current_page->getPageItem(kLoungeTableMain));

   if (!say_item || !table_item)
   {
      return;
   }

   // the server already prepends "nick: " to the message (see Game::processPacket's MESSAGE
   // case) - split it back out here only to avoid repeating the nick on wrapped continuation
   // lines, matching the original's own formatting.
   std::string nick;
   const size_t nick_end = message.find(": ");
   if (nick_end != std::string::npos)
   {
      nick = message.substr(0, nick_end);
   }

   const Constants::Color player_color = BombermanClient::getInstance()->getColor(sender_id);
   Color color = GameSettings::getInstance()->getStyleSettings()->getColor(player_color);
   const Color outline_color(0, 0, 0, 255);

   if (player_color == Constants::ColorBlack)
   {
      color = Color(128, 128, 128, 255);
   }

   const std::vector<std::string> lines = WordWrap::wrap(message, say_item->getFieldWidth());

   int i = 0;
   for (const std::string& line : lines)
   {
      std::string text;
      if (i == 0 || nick.empty())
      {
         text = line;
      }
      else
      {
         text = nick + ": " + line;
      }

      const std::string trimmed = StringUtils::trim(line);
      if (trimmed != nick + ":" && !trimmed.empty())
      {
         table_item->appendItem(text, color, true, outline_color);
      }

      ++i;
   }

   table_item->scrollToPercentage(100.0f, false);
}

void MenuPageNavigator::deserializeLoginData()
{
   // matches GameMenuInterfaceMain::deserializeLoginData().
   MenuPage* page = Menu::getInstance()->getPageByName(kMainMenu);

   auto* host_combo = dynamic_cast<MenuPageEditableComboBoxItem*>(page->getPageItem("editablecombobox_host_table"));
   auto* nick_item = dynamic_cast<MenuPageTextEditItem*>(page->getPageItem("lineedit_nick"));

   if (!host_combo)
   {
      return;
   }

   host_combo->clear();

   const std::string saved_host = GameSettings::getInstance()->getLoginSettings()->getHost();

   const std::vector<std::string> hosts = _host_history.load(saved_host);
   for (const auto& host : hosts)
   {
      host_combo->appendItem(host);
   }

   if (nick_item)
   {
      nick_item->setText(GameSettings::getInstance()->getLoginSettings()->getNick());
   }

   // there is no use to deserialize the saved host when there's already a valid value set up
   // (the game has been started before and a valid hostname restored) - don't overwrite whatever
   // the user already typed.
   auto* host_text_edit = host_combo->getTextEditItem();
   if (host_text_edit && host_text_edit->getText().empty())
   {
      host_text_edit->setText(saved_host);
   }
}

void MenuPageNavigator::deserializeVersion()
{
   // matches GameMenuInterfaceMain::deserializeVersion(). label_version's own PSD opacity is 0
   // (always hidden) - setAlpha() here is what actually makes it visible.
   MenuPage* page = Menu::getInstance()->getPageByName(kMainMenu);

   auto* version = dynamic_cast<MenuPageLabelItem*>(page->getPageItem("label_version"));
   if (!version)
   {
      return;
   }

   version->setText("version " + std::string(GAME_VERSION) + ", revision " + std::string(GAME_REVISION));
   version->setAlpha(100);
}

void MenuPageNavigator::updateLoginData()
{
   // matches GameMenuInterfaceMain::updateLoginData().
   MenuPage* page = Menu::getInstance()->getPageByName(kMainMenu);

   auto* host_combo = dynamic_cast<MenuPageEditableComboBoxItem*>(page->getPageItem("editablecombobox_host_table"));
   auto* nick_item = dynamic_cast<MenuPageTextEditItem*>(page->getPageItem("lineedit_nick"));

   if (!host_combo || !nick_item)
   {
      return;
   }

   auto* host_text_edit = host_combo->getTextEditItem();
   if (!host_text_edit)
   {
      return;
   }

   const std::string host = host_text_edit->getText();
   const std::string nick = nick_item->getText();

   GameSettings::getInstance()->getLoginSettings()->setHost(host);
   GameSettings::getInstance()->getLoginSettings()->setNick(nick);

   _host_history.add(host);
}

void MenuPageNavigator::deserializeCreateGameData()
{
   // matches GameMenuInterfaceCreate::deserializeCreateGameData() - reads from
   // getCreateGameSettingsSingle() unconditionally, unlike initializeCreateGameOptions() below
   // (which does branch on isSinglePlayer()) - this is the original's own behavior, not a typo
   // introduced by the port, so left as-is rather than "fixed" to branch.
   MenuPage* page = Menu::getInstance()->getPageByName(kGameCreate);
   auto* game_name_item = dynamic_cast<MenuPageTextEditItem*>(page->getPageItem("lineedit_name"));

   game_name_item->setText(GameSettings::getInstance()->getCreateGameSettingsSingle()->getGameName());
}

void MenuPageNavigator::initializeCreateGameOptions()
{
   // matches GameMenuInterfaceCreate::initializeCreateGameOptions().
   MenuPage* page = Menu::getInstance()->getPageByName(kGameCreate);

   auto* time_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_time_table"));
   auto* max_players_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_maxplayers_table"));
   auto* bot_count_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_bots_table"));
   auto* level_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_level_table"));
   auto* rounds_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_rounds_table"));

   auto* bomb_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_bomb"));
   auto* flame_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_flame"));
   auto* speed_up_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_speedup"));
   auto* kick_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_kick"));
   auto* skulls_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_skull"));

   if (!_create_game_pages_initialized.contains(page))
   {
      time_combo->appendItem("2");
      time_combo->appendItem("3");
      time_combo->appendItem("5");
      time_combo->appendItem("7");

      for (int i = 0; i < 5; i++)
      {
         rounds_combo->appendItem(std::to_string(i + 1));
      }

      for (int i = 2; i <= 10; i++)
      {
         max_players_combo->appendItem(std::to_string(i));
      }

      for (const std::string& name : _sorted_level_names)
      {
         level_combo->appendItem(name);
      }

      _create_game_pages_initialized.insert(page);
   }

   GameSettings::CreateGameSettings* create_game_settings = BombermanClient::getInstance()->isSinglePlayer()
                                                               ? GameSettings::getInstance()->getCreateGameSettingsSingle()
                                                               : GameSettings::getInstance()->getCreateGameSettingsMulti();

   time_combo->setValue(std::to_string(create_game_settings->getDuration()));
   max_players_combo->setValue(std::to_string(create_game_settings->getMaxPlayers()));
   bot_count_combo->setValue(std::to_string(create_game_settings->getBotCount()));

   int level_index = create_game_settings->getLevelIndex();
   if (static_cast<size_t>(level_index) >= _sorted_level_names.size())
   {
      level_index = 0;
   }

   level_combo->setValue(_sorted_level_names[level_index]);
   level_combo->setActiveElement(level_index);
   level_combo->setFocussedElement(level_index);

   rounds_combo->setValue(std::to_string(create_game_settings->getRounds()));
   bomb_extras_checkbox->setChecked(create_game_settings->isExtraBombsEnabled());
   flame_extras_checkbox->setChecked(create_game_settings->isExtraFlamesEnabled());
   speed_up_extras_checkbox->setChecked(create_game_settings->isExtraSpeedUpsEnabled());
   kick_extras_checkbox->setChecked(create_game_settings->isExtraKicksEnabled());
   skulls_extras_checkbox->setChecked(create_game_settings->isExtraSkullsEnabled());

   updateCreateGamePlayerCounts();
   updateCreateGameLevelPreview();
}

void MenuPageNavigator::updateCreateGamePlayerCounts()
{
   // matches GameMenuInterfaceCreate::updateCreateGamePlayerCounts().
   MenuPage* page = Menu::getInstance()->getPageByName(kGameCreate);

   auto* max_players_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_maxplayers_table"));
   auto* bot_count_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_bots_table"));

   const int max_players = std::atoi(max_players_combo->getValue().c_str());
   const int bot_count = std::atoi(bot_count_combo->getValue().c_str());
   const int max_bots = max_players - 1;

   bot_count_combo->clear();

   for (int i = 0; i <= max_bots; i++)
   {
      bot_count_combo->appendItem(std::to_string(i));
   }

   if (bot_count > max_bots)
   {
      bot_count_combo->setValue(std::to_string(max_bots));
   }
}

void MenuPageNavigator::updateCreateGameLevelPreview()
{
   // matches GameMenuInterfaceCreate::updateCreateGameLevelPreview().
   MenuPage* page = Menu::getInstance()->getPageByName(kGameCreate);

   auto* preview_castle = dynamic_cast<MenuPagePixmapItem*>(page->getPageItem("pixmap_preview_castle"));
   auto* preview_mansion = dynamic_cast<MenuPagePixmapItem*>(page->getPageItem("pixmap_preview_mansion"));
   auto* preview_space = dynamic_cast<MenuPagePixmapItem*>(page->getPageItem("pixmap_preview_space"));
   auto* level_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_level_table"));

   preview_castle->setVisible(level_combo->getFocussedElement() <= 0);
   preview_mansion->setVisible(level_combo->getFocussedElement() == 1);
   preview_space->setVisible(level_combo->getFocussedElement() == 2);
}

void MenuPageNavigator::setMonitorCreateGameOptionsEnabled(bool enabled)
{
   // matches GameMenuInterfaceCreate::setMonitorCreateGameOptionsEnabled().
   if (!enabled && _max_players_value_changed_connection == INVALID_CONNECTION)
   {
      return;
   }

   MenuPage* page = Menu::getInstance()->getPageByName(kGameCreate);

   auto* max_players_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_maxplayers_table"));
   auto* level_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_level_table"));

   if (enabled)
   {
      _max_players_value_changed_connection =
         max_players_combo->valueChangedSignal.connect([this](const std::string&) { updateCreateGamePlayerCounts(); });
      _level_value_changed_connection =
         level_combo->valueChangedSignal.connect([this](const std::string&) { updateCreateGameLevelPreview(); });
      _level_element_focussed_connection = level_combo->elementFocussedSignal.connect([this](int) { updateCreateGameLevelPreview(); });
   }
   else
   {
      max_players_combo->valueChangedSignal.disconnect(_max_players_value_changed_connection);
      level_combo->valueChangedSignal.disconnect(_level_value_changed_connection);
      level_combo->elementFocussedSignal.disconnect(_level_element_focussed_connection);

      _max_players_value_changed_connection = INVALID_CONNECTION;
      _level_value_changed_connection = INVALID_CONNECTION;
      _level_element_focussed_connection = INVALID_CONNECTION;
   }
}

void MenuPageNavigator::deserializeAudioSettings()
{
   // matches GameMenuInterfaceOptions::deserializeAudioSettings() - slider positions come from
   // SoundManager's live volume, not GameSettings directly (SoundManager itself was seeded from
   // GameSettings at startup).
   MenuPage* page = Menu::getInstance()->getPageByName(kOptionsAudio);

   auto* music_slider = dynamic_cast<MenuPageSliderItem*>(page->getPageItem(kOptionsAudioSliderMusic));
   auto* sfx_slider = dynamic_cast<MenuPageSliderItem*>(page->getPageItem(kOptionsAudioSliderSfx));

   music_slider->setValue(SoundManager::getInstance()->getVolumeMusic());
   sfx_slider->setValue(SoundManager::getInstance()->getVolumeSfx());
}

void MenuPageNavigator::setMonitorAudioSettingsEnabled(bool enabled)
{
   // matches GameMenuInterfaceOptions::setMonitorAudioSettingsEnabled() - only the sfx slider
   // gets a tick sound (audible feedback of the new sfx volume itself); the music slider doesn't.
   if (!enabled && _music_volume_changed_connection == INVALID_CONNECTION)
   {
      return;
   }

   MenuPage* page = Menu::getInstance()->getPageByName(kOptionsAudio);

   auto* music_slider = dynamic_cast<MenuPageSliderItem*>(page->getPageItem(kOptionsAudioSliderMusic));
   auto* sfx_slider = dynamic_cast<MenuPageSliderItem*>(page->getPageItem(kOptionsAudioSliderSfx));

   if (enabled)
   {
      _music_volume_changed_connection = music_slider->valueChangedSignal.connect([this](float value) { applyVolumeMusic(value); });
      _sfx_volume_changed_connection = sfx_slider->valueChangedSignal.connect([this](float value) { applyVolumeSfx(value); });
      _sfx_tick_connection = sfx_slider->valueChangedSignal.connect([](float) { SoundManager::getInstance()->playSoundTick(); });
   }
   else
   {
      music_slider->valueChangedSignal.disconnect(_music_volume_changed_connection);
      sfx_slider->valueChangedSignal.disconnect(_sfx_volume_changed_connection);
      sfx_slider->valueChangedSignal.disconnect(_sfx_tick_connection);

      _music_volume_changed_connection = INVALID_CONNECTION;
      _sfx_volume_changed_connection = INVALID_CONNECTION;
      _sfx_tick_connection = INVALID_CONNECTION;
   }
}

void MenuPageNavigator::applyVolumeMusic(float volume)
{
   SoundManager::getInstance()->setVolumeMusic(volume);
}

void MenuPageNavigator::applyVolumeSfx(float volume)
{
   SoundManager::getInstance()->setVolumeSfx(volume);
}

void MenuPageNavigator::restoreAudioDefaults()
{
   // matches GameMenuInterfaceOptions::restoreAudioDefaults(): reset GameSettings, push the
   // (now-default) values into SoundManager, then re-seed the sliders' visual positions.
   GameSettings::AudioSettings* audio_settings = GameSettings::getInstance()->getAudioSettings();
   audio_settings->restoreDefaults();

   SoundManager::getInstance()->setVolumeMusic(audio_settings->getVolumeMusic());
   SoundManager::getInstance()->setVolumeSfx(audio_settings->getVolumeSfx());

   deserializeAudioSettings();
}

void MenuPageNavigator::createGame()
{
   // matches GameMenuInterfaceCreate::createGame().
   MenuPage* page = Menu::getInstance()->getPageByName(kGameCreate);

   auto* game_name_item = dynamic_cast<MenuPageTextEditItem*>(page->getPageItem("lineedit_name"));
   auto* time_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_time_table"));
   auto* max_players_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_maxplayers_table"));
   auto* bot_count_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_bots_table"));
   auto* level_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_level_table"));
   auto* rounds_combo = dynamic_cast<MenuPageComboBoxItem*>(page->getPageItem("combobox_rounds_table"));

   auto* bomb_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_bomb"));
   auto* flame_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_flame"));
   auto* speed_up_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_speedup"));
   auto* kick_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_kick"));
   auto* skulls_extras_checkbox = dynamic_cast<MenuPageCheckBoxItem*>(page->getPageItem("checkbox_skull"));

   const std::string game_name = game_name_item->getText();

   int level_index = level_combo->getActiveElement();
   std::string level_dir_name = (static_cast<size_t>(level_index) >= _sorted_level_dir_names.size()) ? _sorted_level_dir_names[0]
                                                                                                     : _sorted_level_dir_names[level_index];

   const int duration_minutes = std::atoi(time_combo->getValue().c_str());
   const int duration_seconds = duration_minutes * 60;
   const int max_players = std::atoi(max_players_combo->getValue().c_str());
   const int bot_count = std::atoi(bot_count_combo->getValue().c_str());
   const int rounds = std::atoi(rounds_combo->getValue().c_str());

   const bool extra_bombs = bomb_extras_checkbox->isChecked();
   const bool extra_flames = flame_extras_checkbox->isChecked();
   const bool extra_speed_ups = speed_up_extras_checkbox->isChecked();
   const bool extra_kicks = kick_extras_checkbox->isChecked();
   const bool extra_skulls = skulls_extras_checkbox->isChecked();

   const Constants::Dimension dimension = (max_players <= 5) ? Constants::Dimension13x11 : Constants::Dimension19x17;

   GameSettings::CreateGameSettings* create_game_settings = BombermanClient::getInstance()->isSinglePlayer()
                                                               ? GameSettings::getInstance()->getCreateGameSettingsSingle()
                                                               : GameSettings::getInstance()->getCreateGameSettingsMulti();

   create_game_settings->setGameName(game_name);
   create_game_settings->setLevelIndex(level_index);
   create_game_settings->setRounds(rounds);
   create_game_settings->setDuration(duration_minutes);
   create_game_settings->setMaxPlayers(max_players);
   create_game_settings->setExtraBombsEnabled(extra_bombs);
   create_game_settings->setExtraFlamesEnabled(extra_flames);
   create_game_settings->setExtraKicksEnabled(extra_kicks);
   create_game_settings->setExtraSpeedUpsEnabled(extra_speed_ups);
   create_game_settings->setExtraSkullsEnabled(extra_skulls);
   create_game_settings->setDimensions(dimension);
   create_game_settings->setBotCount(bot_count);
   create_game_settings->serialize();

   BombermanClient::getInstance()->createGame(
      game_name,
      level_dir_name,
      rounds,
      duration_seconds,
      max_players,
      extra_bombs,
      extra_flames,
      extra_speed_ups,
      extra_kicks,
      extra_skulls,
      dimension
   );
}
