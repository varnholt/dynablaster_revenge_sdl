#include "menupagenavigator.h"

#include "game/bombermanclient.h"
#include "game/gamesettings.h"
#include "game/gameversion.h"
#include "game/soundmanager.h"
#include "game/videooptions.h"
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

#include <SDL3/SDL_keyboard.h>
#include <SDL3_net/SDL_net.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <format>
#include <memory>
#include <span>
#include <string_view>

namespace
{

// Mirrors client/src/menus/gamemenudefines.h's page/action string constants. Not #include-d
// directly - that header also defines networking-only action names this class deliberately
// doesn't handle, and pulling it in would imply more coverage than actually exists here.
const std::string kMainMenu = "data/menus/mainmenu.psd";
const std::string kMainMenuActionOptions = "button_options_active";
const std::string kMainMenuActionAbout = "button_about_active";
const std::string kMainMenuActionQuit = "button_quit_active";

const std::string kGameSelect = "data/menus/selectgame.psd";
const std::string kGameSelectActionCreate = "button_create_active";
const std::string kGameSelectActionBack = "button_back_active";

const std::string kGameCreate = "data/menus/creategame.psd";
const std::string kGameCreateActionCancel = "button_cancel_active";

const std::string kOptionsVideo = "data/menus/options_video.psd";
const std::string kOptionsAudio = "data/menus/options_audio.psd";
const std::string kOptionsControls = "data/menus/options_controls.psd";
const std::string kOptionsGame = "data/menus/options_game.psd";
const std::string kOptionsActionOk = "button_ok_active";
const std::string kOptionsActionCancel = "button_cancel_active";
const std::string kOptionsActionVideo = "button_video_active";
const std::string kOptionsActionAudio = "button_audio_active";
const std::string kOptionsActionControls = "button_controls_active";
const std::string kOptionsActionGame = "button_game_active";
const std::string kOptionsVideoActionRestoreDefaults = "button_default_active";
const std::string kOptionsVideoComboResolution = "combobox_resolution_table";
const std::string kOptionsVideoComboDisplayMode = "combobox_display_table";
const std::string kOptionsVideoComboAntialias = "combobox_antialiasing_table";
const std::string kOptionsVideoComboVsync = "combobox_vsync_table";
const std::string kOptionsVideoCheckBoxFps = "checkbox_fps";
const std::string kOptionsVideoSliderBrightness = "slider_brightness";
const std::string kOptionsAudioActionRestoreDefaults = "button_default_active";
const std::string kOptionsGameActionRestoreDefaults = "button_default_active";
const std::string kOptionsGameCheckBoxCameraFollows = "checkbox_cfollow";
const std::string kOptionsGameSliderShake = "slider_shake";
const std::string kOptionsControlsActionRestoreDefaults = "button_default_active";

struct KeyField
{
   std::string_view item;
   Constants::Key key;
};

const std::array<KeyField, 5> kOptionsControlsKeyFields{{
   {"lineedit_keyboard_up", Constants::KeyUp},
   {"lineedit_keyboard_down", Constants::KeyDown},
   {"lineedit_keyboard_left", Constants::KeyLeft},
   {"lineedit_keyboard_right", Constants::KeyRight},
   {"lineedit_keyboard_bomb", Constants::KeyBomb},
}};
const std::string kOptionsAudioSliderMusic = "slider_music";
const std::string kOptionsAudioSliderSfx = "slider_game";

const std::string kAbout = "data/menus/about.psd";
const std::string kAboutActionBack = "button_back_active";
const std::string kLounge = "data/menus/lounge.psd";
const std::string kLoungeActionStart = "button_start_active";
const std::string kLoungeActionBack = "button_leave_active";
const std::string kLoungeAddPlayer = "button_addplayer";
const std::string kLoungeLineeditSay = "lineedit_say";
const std::string kLoungeTableMain = "table_lounge_main";

const std::string kMainMenuActionSingle = "button_single_active";

// the orange led of the login window, a hidden way into the story mode
const std::string kMainMenuActionStory = "led_orange";
const std::string kMainMenuActionMulti = "button_multi_active";
const std::string kMainMenuActionPouet = "button_pouet_active";
const std::string kMainMenuActionFacebook = "button_facebook_active";
const std::string kMainMenuActionHome = "button_home_active";
const std::string kGameSelectActionJoin = "button_join_active";
const std::string kGameCreateActionOk = "button_ok_active";

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
   const std::unique_ptr<NET_Address*[], decltype(&NET_FreeLocalAddresses)> addresses(
      NET_GetLocalAddresses(&count), &NET_FreeLocalAddresses
   );

   if (addresses)
   {
      for (NET_Address* address : std::span(addresses.get(), static_cast<size_t>(count)))
      {
         if (const char* text = NET_GetAddressString(address); text && host_name == text)
         {
            local = true;
            break;
         }
      }
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
   BombermanClient& client = BombermanClient::getInstance();
   _login_response_connection = client.loginResponseSignal.connect([this](bool granted) { onLoginResponse(granted); });
   _create_game_response_connection = client.createGameResponseSignal.connect([this](bool granted, int game_id, bool owner)
                                                                              { onCreateGameResponse(granted, game_id, owner); });
   _join_game_response_connection = client.joinGameResponseSignal.connect([this](bool success) { onJoinGameResponse(success); });
   _game_started_connection = client.gameStartedSignal.connect([this]() { onGameStarted(); });

   // matches GameMenuInterfaceLounge's constructor connection - keeps the lounge's player rows
   // (nick/wins/rank/owner-icon) live-updated whenever the player set changes (join/leave/bot
   // added). Without this, bots that join after the lounge page is already showing never appear.
   _player_info_map_updated_connection =
      client.playerInfoMapUpdatedSignal.connect([this](const std::map<int, PlayerInfo>& info_map) { onPlayerInfoMapUpdated(info_map); });

   // matches GameMenuWorkflow's own connection to BombermanClient::messageReceived - lounge chat.
   _message_received_connection = client.messageReceivedSignal.connect([this](int sender_id, const std::string& message, bool finished)
                                                                       { onMessageReceived(sender_id, message, finished); });

   // matches MenuWorkflow::initialize() calling deserializeLoginData() once at startup - the
   // main menu is already the current page by construction time (no pageChanged() fires for it
   // the very first time), so this can't wait for onPageChanged()'s own kMainMenu branch below.
   deserializeLoginData();
   deserializeVersion();
}

MenuPageNavigator::~MenuPageNavigator()
{
   // only still connected while GAME_CREATE / OPTIONS_VIDEO / OPTIONS_AUDIO is the current page
   setMonitorCreateGameOptionsEnabled(false);
   setMonitorVideoSettingsEnabled(false);
   setMonitorAudioSettingsEnabled(false);

   if (BombermanClient::hasInstance())
   {
      BombermanClient& client = BombermanClient::getInstance();
      client.loginResponseSignal.disconnect(_login_response_connection);
      client.createGameResponseSignal.disconnect(_create_game_response_connection);
      client.joinGameResponseSignal.disconnect(_join_game_response_connection);
      client.gameStartedSignal.disconnect(_game_started_connection);
      client.playerInfoMapUpdatedSignal.disconnect(_player_info_map_updated_connection);
      client.messageReceivedSignal.disconnect(_message_received_connection);
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
         BombermanClient::getInstance().setGameMode(Constants::GameModeSinglePlayer);
         BombermanClient::getInstance().host();
         BombermanClient::getInstance().loginRequest("127.0.0.1", GameSettings::getInstance().getLoginSettings().getNick());
      }
      else if (action == kMainMenuActionStory)
      {
         // a single player game against the original's 64 stages, also on an in-process server
         BombermanClient::getInstance().setGameMode(Constants::GameModeStory);
         BombermanClient::getInstance().host();
         BombermanClient::getInstance().loginRequest("127.0.0.1", GameSettings::getInstance().getLoginSettings().getNick());
      }
      else if (action == kMainMenuActionMulti)
      {
         // matches GameMenuWorkflow's MAINMENU_ACTION_MULTI handler: only hosts if the
         // configured host is actually this machine, always logs in to whatever host is set.
         BombermanClient::getInstance().setGameMode(Constants::GameModeMultiPlayer);

         const std::string host = GameSettings::getInstance().getLoginSettings().getHost();

         if (isHostLocal(host))
         {
            BombermanClient::getInstance().host();
         }

         BombermanClient::getInstance().loginRequest(host, GameSettings::getInstance().getLoginSettings().getNick());
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
         const std::vector<GameInformation>& games = BombermanClient::getInstance().getGames();
         if (!games.empty())
         {
            requestJoin(games.front().getId(), kGameSelect);
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
         // video: OK stores and applies the video page (any tab's OK, like storeOptions()),
         // Cancel restores the backup taken when it was first shown, undoing a brightness drag.
         // controls/game: OK stores what their pages show, Cancel just doesn't - and reloads the
         // stored steering axes the stick calibration may have changed.
         if (action == kOptionsActionOk && _gameplay_settings_shown)
         {
            serializeGameplaySettings();
         }

         if (_controller_settings_shown)
         {
            if (action == kOptionsActionOk)
            {
               serializeControllerSettings();
            }
            else
            {
               GameSettings::getInstance().getControllerSettings().deserialize();
            }
         }

         _gameplay_settings_shown = false;
         _controller_settings_shown = false;

         if (_video_settings_shown)
         {
            if (action == kOptionsActionOk)
            {
               serializeVideoSettings();
            }
            else
            {
               GameSettings::VideoSettings::duplicate(
                  GameSettings::getInstance().getVideoSettings(), GameSettings::getInstance().getVideoSettingsBackup()
               );
            }

            _video_settings_shown = false;
         }

         if (page == kOptionsAudio)
         {
            GameSettings::AudioSettings& audio_settings = GameSettings::getInstance().getAudioSettings();

            if (action == kOptionsActionOk)
            {
               audio_settings.setVolumeMusic(SoundManager::getInstance().getVolumeMusic());
               audio_settings.setVolumeSfx(SoundManager::getInstance().getVolumeSfx());
               audio_settings.serialize();
            }
            else
            {
               SoundManager::getInstance().setVolumeMusic(audio_settings.getVolumeMusic());
               SoundManager::getInstance().setVolumeSfx(audio_settings.getVolumeSfx());
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
      else if (page == kOptionsGame && action == kOptionsGameActionRestoreDefaults)
      {
         restoreGameDefaults();
      }
      else if (page == kOptionsControls && action == kOptionsControlsActionRestoreDefaults)
      {
         restoreControlsDefaults();
      }
      else if (page == kOptionsVideo && action == kOptionsVideoActionRestoreDefaults)
      {
         restoreVideoDefaults();
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
         BombermanClient::getInstance().startGame(BombermanClient::getInstance().getGameId());
      }
      else if (action == kLoungeActionBack)
      {
         BombermanClient::getInstance().leaveGameRequest();
         pageChangeRequestSignal(kGameSelect);
      }
      else if (action == kLoungeLineeditSay)
      {
         // matches GameMenuWorkflow::onActionRequest()'s LOUNGE_LINEEDIT_SAY branch - MenuPage::
         // keyPressed() (menupage.cpp) only emits this actionRequest on Return/Enter, so this is
         // the "finished typing, send it" path. Per-keystroke "still typing" notifications would
         // need MenuPage::actionKeyPressed() wired up too - not done here, matches the existing
         // "typing bubble never shown" simplification (see updateLoungePlayerList()).
         MenuPage& lounge_page = Menu::getInstance().getPageByName(kLounge)->get();

         if (const auto say_item = lounge_page.getPageItem<MenuPageTextEditItem>(kLoungeLineeditSay))
         {
            const std::string message = say_item->get().getText();

            if (!StringUtils::trim(message).empty())
            {
               // lounge chat always broadcasts to everyone (receiverId -1)
               BombermanClient::getInstance().sendMessage(message, true);
               say_item->get().setText("");
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
      const Constants::GameMode mode = BombermanClient::getInstance().getGameMode();

      // story games have nothing to set up
      if (mode == Constants::GameModeStory)
      {
         // development runs may start at a later stage
         const char* first_stage = std::getenv("DYNABLASTER_STORY_STAGE");
         BombermanClient::getInstance().createStoryGame(first_stage ? std::atoi(first_stage) : 0);
         return;
      }

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
   if (granted && owner && BombermanClient::getInstance().getGameMode() == Constants::GameModeStory)
   {
      BombermanClient::getInstance().joinGame(game_id);
   }
   else if (granted && owner)
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
   // no lounge for the story mode, the server waits for the level to be loaded
   if (success && BombermanClient::getInstance().getGameMode() == Constants::GameModeStory)
   {
      BombermanClient::getInstance().startGame(BombermanClient::getInstance().getGameId());
      return;
   }

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
   setMonitorVideoSettingsEnabled(false);
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
      updateLoungePlayerList(BombermanClient::getInstance().getPlayerInfoMap());

      // players on this machine are set up on the controls page before joining
      if (const auto add_player = Menu::getInstance().getPageByName(kLounge)->get().getPageItem(kLoungeAddPlayer))
      {
         add_player->get().setVisible(false);
      }
   }
   else if (page == kOptionsGame)
   {
      deserializeGameplaySettings();
   }
   else if (page == kOptionsControls)
   {
      deserializeControllerSettings();
   }
   else if (page == kOptionsVideo)
   {
      deserializeVideoSettings();
      setMonitorVideoSettingsEnabled(true);
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

void MenuPageNavigator::setVideoSettingsHandler(std::function<void()> handler)
{
   _video_settings_handler = std::move(handler);
}

std::optional<std::reference_wrapper<MenuPageTextEditItem>> MenuPageNavigator::getActiveKeyField() const
{
   const auto page = Menu::getInstance().getCurrentPage();
   if (!page || page->get().getFilename() != kOptionsControls)
   {
      return std::nullopt;
   }

   const auto item = page->get().getActiveItem();
   if (!item || !item->get().getCurrentLayer())
   {
      return std::nullopt;
   }

   const auto& name = item->get().getCurrentLayer()->get().getName();
   const bool is_key_field = std::ranges::any_of(kOptionsControlsKeyFields, [&name](const KeyField& field) { return name == field.item; });

   if (!is_key_field)
   {
      return std::nullopt;
   }

   if (auto* field = dynamic_cast<MenuPageTextEditItem*>(&item->get()))
   {
      return *field;
   }
   return std::nullopt;
}

bool MenuPageNavigator::onKeyPressed(int key)
{
   // matches processOptionsControlKeyPressed(): the field shows the key's name. escape and tab
   // keep their menu meaning
   const auto field = getActiveKeyField();
   if (!field || key == SDLK_ESCAPE || key == SDLK_TAB)
   {
      return false;
   }

   const std::string name = SDL_GetKeyName(static_cast<SDL_Keycode>(key));
   if (!name.empty())
   {
      field->get().setText(name);
      field->get().setCursorPosition(static_cast<int>(name.length()));
   }

   return true;
}

bool MenuPageNavigator::onTextInput()
{
   return getActiveKeyField().has_value();
}

void MenuPageNavigator::initializeBots(int32_t remaining_tries)
{
   if (remaining_tries > 0 && _bots_wait_condition && _bots_wait_condition())
   {
      Timer::singleShot(250, [this, remaining_tries]() { initializeBots(remaining_tries - 1); });
      return;
   }

   BombermanClient::getInstance().initializeBots();
}

void MenuPageNavigator::requestJoin(int game_id, const std::string& return_page)
{
   if (!_join_handler || !_join_handler(game_id, return_page))
   {
      BombermanClient::getInstance().joinGame(game_id);
   }
}

void MenuPageNavigator::onPlayerInfoMapUpdated(const std::map<int, PlayerInfo>& player_info)
{
   updateLoungePlayerList(player_info);
}

void MenuPageNavigator::updateLoungePlayerList(const std::map<int, PlayerInfo>& player_info)
{
   // matches GameMenuInterfaceLounge::playerInfoMapUpdated() - only touches the UI while the
   // lounge page is actually the one showing (mirrors the original's own current_page == page
   // guard, since this can also fire while some other page, e.g. main menu after leaving, is up).
   const auto current = Menu::getInstance().getCurrentPage();
   const auto lounge = Menu::getInstance().getPageByName(kLounge);

   if (!current || !lounge || &current->get() != &lounge->get())
   {
      return;
   }

   MenuPage& current_page = *current;

   _player_id_to_index_map.clear();

   struct ScoreEntry
   {
      std::reference_wrapper<const PlayerInfo> player;
      int score;
   };

   std::vector<ScoreEntry> score_list;
   for (const auto& [id, info] : player_info)
   {
      score_list.push_back({info, static_cast<int>(info.getOverallStats().getWins())});
   }

   std::sort(score_list.begin(), score_list.end(), [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });

   const auto set_visible = [&current_page](const std::string& name, bool visible)
   {
      if (const auto item = current_page.getPageItem(name))
      {
         item->get().setVisible(visible);
      }
   };

   // initially hide all rows
   for (int i = 1; i <= 10; i++)
   {
      if (const auto nick_item = current_page.getPageItem<MenuPageLabelItem>("label_p" + std::to_string(i)))
      {
         nick_item->get().setText("");
      }

      set_visible("p" + std::to_string(i) + "_box_active", false);
      set_visible("p" + std::to_string(i) + "_leader_icon", false);
      set_visible("p" + std::to_string(i) + "_icon", false);
      set_visible("label_rank_" + std::to_string(i), false);

      if (const auto wins_item = current_page.getPageItem<MenuPageLabelItem>("label_p" + std::to_string(i) + "_wins"))
      {
         wins_item->get().setVisible(false);
      }

      // matches GameMenuInterfaceLounge::initializeLoungeStates() - only ever shown by the
      // typing-indicator feature (updatePlayerTyping()/removePlayerTyping()), which isn't ported
      // (see the deferred chat handling below) - so it must be force-hidden here instead, or the
      // PSD's raw default visibility leaks through unmodified for every row.
      set_visible("p" + std::to_string(i) + "_box_type", false);
   }

   int counter = 0;
   for (const ScoreEntry& entry : score_list)
   {
      counter++;

      const PlayerInfo& player = entry.player;
      const int color = static_cast<int>(player.getColor());
      const auto game_info = BombermanClient::getInstance().getCurrentGameInformation();
      const bool owner = game_info && (player.getId() == game_info->get().getCreatorId());

      // note: the original also nudges player_item's active-layer Y to align with active_box_item's
      // top (player_item->getCurrentLayer()->setY(...)) - PSDLayer::setY() isn't ported in this
      // port (only getters + setOpacity exist), so that pixel-alignment tweak is skipped; the row
      // still shows correctly, just not pixel-perfect vertically.

      _player_id_to_index_map[player.getId()] = counter;

      if (const auto wins_item = current_page.getPageItem<MenuPageLabelItem>("label_p" + std::to_string(counter) + "_wins"))
      {
         wins_item->get().setText(std::to_string(entry.score));
         wins_item->get().setColor(Color(counter <= 3 ? "#fbfe00" : "#3b7d9d"));
         wins_item->get().setVisible(true);
      }

      if (const auto nick_item = current_page.getPageItem<MenuPageLabelItem>("label_p" + std::to_string(counter)))
      {
         nick_item->get().setText(player.getNick());
      }

      set_visible("p" + std::to_string(counter) + "_box_active", true);
      set_visible("p" + std::to_string(counter) + "_leader_icon", owner);
      set_visible("p" + std::to_string(color) + "_icon", true);
      set_visible("label_rank_" + std::to_string(counter), true);
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
   const auto lounge_page = Menu::getInstance().getPageByName(kLounge);
   const auto current_page = Menu::getInstance().getCurrentPage();

   if (!lounge_page || !current_page || &lounge_page->get() != &current_page->get())
   {
      return;
   }

   const auto say = lounge_page->get().getPageItem<MenuPageTextEditItem>(kLoungeLineeditSay);
   const auto table = current_page->get().getPageItem<MenuPageListItem>(kLoungeTableMain);

   if (!say || !table)
   {
      return;
   }

   MenuPageTextEditItem& say_item = *say;
   MenuPageListItem& table_item = *table;

   // the server already prepends "nick: " to the message (see Game::processPacket's MESSAGE
   // case) - split it back out here only to avoid repeating the nick on wrapped continuation
   // lines, matching the original's own formatting.
   std::string nick;
   const size_t nick_end = message.find(": ");
   if (nick_end != std::string::npos)
   {
      nick = message.substr(0, nick_end);
   }

   const Constants::Color player_color = BombermanClient::getInstance().getColor(sender_id);
   Color color = GameSettings::getInstance().getStyleSettings().getColor(player_color);
   const Color outline_color(0, 0, 0, 255);

   if (player_color == Constants::ColorBlack)
   {
      color = Color(128, 128, 128, 255);
   }

   const std::vector<std::string> lines = WordWrap::wrap(message, say_item.getFieldWidth());

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
         table_item.appendItem(text, color, true, outline_color);
      }

      ++i;
   }

   table_item.scrollToPercentage(100.0f, false);
}

void MenuPageNavigator::deserializeLoginData()
{
   // matches GameMenuInterfaceMain::deserializeLoginData().
   MenuPage& page = Menu::getInstance().getPageByName(kMainMenu)->get();

   const auto host_combo_item = page.getPageItem<MenuPageEditableComboBoxItem>("editablecombobox_host_table");
   const auto nick_item = page.getPageItem<MenuPageTextEditItem>("lineedit_nick");

   if (!host_combo_item)
   {
      return;
   }

   MenuPageEditableComboBoxItem& host_combo = *host_combo_item;
   host_combo.clear();

   const std::string saved_host = GameSettings::getInstance().getLoginSettings().getHost();

   const std::vector<std::string> hosts = _host_history.load(saved_host);
   for (const auto& host : hosts)
   {
      host_combo.appendItem(host);
   }

   if (nick_item)
   {
      nick_item->get().setText(GameSettings::getInstance().getLoginSettings().getNick());
   }

   // there is no use to deserialize the saved host when there's already a valid value set up
   // (the game has been started before and a valid hostname restored) - don't overwrite whatever
   // the user already typed.
   const auto host_text_edit = host_combo.getTextEditItem();
   if (host_text_edit && host_text_edit->get().getText().empty())
   {
      host_text_edit->get().setText(saved_host);
   }
}

void MenuPageNavigator::deserializeVersion()
{
   // matches GameMenuInterfaceMain::deserializeVersion(). label_version's own PSD opacity is 0
   // (always hidden) - setAlpha() here is what actually makes it visible.
   MenuPage& page = Menu::getInstance().getPageByName(kMainMenu)->get();

   const auto version = page.getPageItem<MenuPageLabelItem>("label_version");
   if (!version)
   {
      return;
   }

   version->get().setText("version " + std::string(GAME_VERSION) + ", revision " + std::string(GAME_REVISION));
   version->get().setAlpha(100);
}

void MenuPageNavigator::updateLoginData()
{
   // matches GameMenuInterfaceMain::updateLoginData().
   MenuPage& page = Menu::getInstance().getPageByName(kMainMenu)->get();

   const auto host_combo = page.getPageItem<MenuPageEditableComboBoxItem>("editablecombobox_host_table");
   const auto nick_item = page.getPageItem<MenuPageTextEditItem>("lineedit_nick");

   if (!host_combo || !nick_item)
   {
      return;
   }

   const auto host_text_edit = host_combo->get().getTextEditItem();
   if (!host_text_edit)
   {
      return;
   }

   const std::string host = host_text_edit->get().getText();
   const std::string nick = nick_item->get().getText();

   GameSettings::getInstance().getLoginSettings().setHost(host);
   GameSettings::getInstance().getLoginSettings().setNick(nick);

   _host_history.add(host);
}

void MenuPageNavigator::deserializeCreateGameData()
{
   // matches GameMenuInterfaceCreate::deserializeCreateGameData() - reads from
   // getCreateGameSettingsSingle() unconditionally, unlike initializeCreateGameOptions() below
   // (which does branch on isSinglePlayer()) - this is the original's own behavior, not a typo
   // introduced by the port, so left as-is rather than "fixed" to branch.
   MenuPage& page = Menu::getInstance().getPageByName(kGameCreate)->get();
   MenuPageTextEditItem& game_name_item = page.getPageItem<MenuPageTextEditItem>("lineedit_name")->get();

   game_name_item.setText(GameSettings::getInstance().getCreateGameSettingsSingle().getGameName());
}

void MenuPageNavigator::initializeCreateGameOptions()
{
   // matches GameMenuInterfaceCreate::initializeCreateGameOptions().
   MenuPage& page = Menu::getInstance().getPageByName(kGameCreate)->get();

   MenuPageComboBoxItem& time_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_time_table")->get();
   MenuPageComboBoxItem& max_players_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_maxplayers_table")->get();
   MenuPageComboBoxItem& bot_count_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_bots_table")->get();
   MenuPageComboBoxItem& level_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_level_table")->get();
   MenuPageComboBoxItem& rounds_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_rounds_table")->get();

   MenuPageCheckBoxItem& bomb_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_bomb")->get();
   MenuPageCheckBoxItem& flame_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_flame")->get();
   MenuPageCheckBoxItem& speed_up_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_speedup")->get();
   MenuPageCheckBoxItem& kick_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_kick")->get();
   MenuPageCheckBoxItem& skulls_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_skull")->get();

   if (!_create_game_options_initialized)
   {
      time_combo.appendItem("2");
      time_combo.appendItem("3");
      time_combo.appendItem("5");
      time_combo.appendItem("7");

      for (int i = 0; i < 5; i++)
      {
         rounds_combo.appendItem(std::to_string(i + 1));
      }

      for (int i = 2; i <= 10; i++)
      {
         max_players_combo.appendItem(std::to_string(i));
      }

      for (const std::string& name : _sorted_level_names)
      {
         level_combo.appendItem(name);
      }

      _create_game_options_initialized = true;
   }

   GameSettings::CreateGameSettings& create_game_settings = BombermanClient::getInstance().isSinglePlayer()
                                                               ? GameSettings::getInstance().getCreateGameSettingsSingle()
                                                               : GameSettings::getInstance().getCreateGameSettingsMulti();

   time_combo.setValue(std::to_string(create_game_settings.getDuration()));
   max_players_combo.setValue(std::to_string(create_game_settings.getMaxPlayers()));
   bot_count_combo.setValue(std::to_string(create_game_settings.getBotCount()));

   int level_index = create_game_settings.getLevelIndex();
   if (static_cast<size_t>(level_index) >= _sorted_level_names.size())
   {
      level_index = 0;
   }

   level_combo.setValue(_sorted_level_names[level_index]);
   level_combo.setActiveElement(level_index);
   level_combo.setFocussedElement(level_index);

   rounds_combo.setValue(std::to_string(create_game_settings.getRounds()));
   bomb_extras_checkbox.setChecked(create_game_settings.isExtraBombsEnabled());
   flame_extras_checkbox.setChecked(create_game_settings.isExtraFlamesEnabled());
   speed_up_extras_checkbox.setChecked(create_game_settings.isExtraSpeedUpsEnabled());
   kick_extras_checkbox.setChecked(create_game_settings.isExtraKicksEnabled());
   skulls_extras_checkbox.setChecked(create_game_settings.isExtraSkullsEnabled());

   updateCreateGamePlayerCounts();
   updateCreateGameLevelPreview();
}

void MenuPageNavigator::updateCreateGamePlayerCounts()
{
   // matches GameMenuInterfaceCreate::updateCreateGamePlayerCounts().
   MenuPage& page = Menu::getInstance().getPageByName(kGameCreate)->get();

   MenuPageComboBoxItem& max_players_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_maxplayers_table")->get();
   MenuPageComboBoxItem& bot_count_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_bots_table")->get();

   const int max_players = std::atoi(max_players_combo.getValue().c_str());
   const int bot_count = std::atoi(bot_count_combo.getValue().c_str());
   const int max_bots = max_players - 1;

   bot_count_combo.clear();

   for (int i = 0; i <= max_bots; i++)
   {
      bot_count_combo.appendItem(std::to_string(i));
   }

   if (bot_count > max_bots)
   {
      bot_count_combo.setValue(std::to_string(max_bots));
   }
}

void MenuPageNavigator::updateCreateGameLevelPreview()
{
   // matches GameMenuInterfaceCreate::updateCreateGameLevelPreview().
   MenuPage& page = Menu::getInstance().getPageByName(kGameCreate)->get();

   MenuPagePixmapItem& preview_castle = page.getPageItem<MenuPagePixmapItem>("pixmap_preview_castle")->get();
   MenuPagePixmapItem& preview_mansion = page.getPageItem<MenuPagePixmapItem>("pixmap_preview_mansion")->get();
   MenuPagePixmapItem& preview_space = page.getPageItem<MenuPagePixmapItem>("pixmap_preview_space")->get();
   MenuPageComboBoxItem& level_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_level_table")->get();

   preview_castle.setVisible(level_combo.getFocussedElement() <= 0);
   preview_mansion.setVisible(level_combo.getFocussedElement() == 1);
   preview_space.setVisible(level_combo.getFocussedElement() == 2);
}

void MenuPageNavigator::setMonitorCreateGameOptionsEnabled(bool enabled)
{
   // matches GameMenuInterfaceCreate::setMonitorCreateGameOptionsEnabled().
   if (!enabled && _max_players_value_changed_connection == INVALID_CONNECTION)
   {
      return;
   }

   MenuPage& page = Menu::getInstance().getPageByName(kGameCreate)->get();

   MenuPageComboBoxItem& max_players_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_maxplayers_table")->get();
   MenuPageComboBoxItem& level_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_level_table")->get();

   if (enabled)
   {
      _max_players_value_changed_connection =
         max_players_combo.valueChangedSignal.connect([this](const std::string&) { updateCreateGamePlayerCounts(); });
      _level_value_changed_connection =
         level_combo.valueChangedSignal.connect([this](const std::string&) { updateCreateGameLevelPreview(); });
      _level_element_focussed_connection = level_combo.elementFocussedSignal.connect([this](int) { updateCreateGameLevelPreview(); });
   }
   else
   {
      max_players_combo.valueChangedSignal.disconnect(_max_players_value_changed_connection);
      level_combo.valueChangedSignal.disconnect(_level_value_changed_connection);
      level_combo.elementFocussedSignal.disconnect(_level_element_focussed_connection);

      _max_players_value_changed_connection = INVALID_CONNECTION;
      _level_value_changed_connection = INVALID_CONNECTION;
      _level_element_focussed_connection = INVALID_CONNECTION;
   }
}

void MenuPageNavigator::deserializeVideoSettings()
{
   GameSettings::VideoSettings& settings = GameSettings::getInstance().getVideoSettings();

   // later visits (e.g. video -> audio -> video) keep the first backup, so Cancel still undoes them
   if (!_video_settings_shown)
   {
      GameSettings::VideoSettings::duplicate(GameSettings::getInstance().getVideoSettingsBackup(), settings);
      _video_settings_shown = true;
   }

   MenuPage& page = Menu::getInstance().getPageByName(kOptionsVideo)->get();

   MenuPageComboBoxItem& resolution_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboResolution)->get();
   MenuPageComboBoxItem& display_mode_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboDisplayMode)->get();
   MenuPageComboBoxItem& antialias_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboAntialias)->get();
   MenuPageComboBoxItem& vsync_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboVsync)->get();
   MenuPageCheckBoxItem& fps_checkbox = page.getPageItem<MenuPageCheckBoxItem>(kOptionsVideoCheckBoxFps)->get();
   MenuPageSliderItem& brightness_slider = page.getPageItem<MenuPageSliderItem>(kOptionsVideoSliderBrightness)->get();

   const auto fill = [](MenuPageComboBoxItem& combo, const std::vector<std::string>& texts, size_t active)
   {
      combo.clear();

      for (const auto& text : texts)
      {
         combo.appendItem(text);
      }

      const auto index = static_cast<int>(active < texts.size() ? active : 0);
      combo.setValue(combo.getElementText(index));
      combo.setActiveElement(index);
      combo.setFocussedElement(index);
   };

   // values.size() if missing, fill() then picks the first entry
   const auto index_of = [](const std::vector<int32_t>& values, int32_t value)
   { return static_cast<size_t>(std::distance(values.begin(), std::ranges::find(values, value))); };

   const auto resolutions = VideoOptions::resolutions();
   std::vector<std::string> resolution_texts;
   for (const auto resolution : resolutions)
   {
      resolution_texts.push_back(std::format("{} x {}", resolution, resolution));
   }
   fill(resolution_combo, resolution_texts, index_of(resolutions, settings.getResolution()));

   fill(display_mode_combo, {"fullscreen", "windowed"}, settings.isFullscreen() ? 0 : 1);

   const auto sample_counts = VideoOptions::sampleCounts();
   std::vector<std::string> sample_texts;
   for (const auto samples : sample_counts)
   {
      sample_texts.push_back(std::format("{}x", samples));
   }
   fill(antialias_combo, sample_texts, index_of(sample_counts, settings.getAntialias()));

   // the index is the swap interval
   fill(vsync_combo, {"off", "60fps", "30fps"}, static_cast<size_t>(settings.getVSync()));

   fps_checkbox.setChecked(settings.isFpsShown());
   brightness_slider.setValue(settings.getBrightness());
}

void MenuPageNavigator::serializeVideoSettings()
{
   MenuPage& page = Menu::getInstance().getPageByName(kOptionsVideo)->get();

   MenuPageComboBoxItem& resolution_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboResolution)->get();
   MenuPageComboBoxItem& display_mode_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboDisplayMode)->get();
   MenuPageComboBoxItem& antialias_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboAntialias)->get();
   MenuPageComboBoxItem& vsync_combo = page.getPageItem<MenuPageComboBoxItem>(kOptionsVideoComboVsync)->get();
   MenuPageCheckBoxItem& fps_checkbox = page.getPageItem<MenuPageCheckBoxItem>(kOptionsVideoCheckBoxFps)->get();

   const auto value_at = [](const std::vector<int32_t>& values, int index)
   { return (index >= 0 && static_cast<size_t>(index) < values.size()) ? values[static_cast<size_t>(index)] : 1; };

   GameSettings::VideoSettings& settings = GameSettings::getInstance().getVideoSettings();
   settings.setResolution(value_at(VideoOptions::resolutions(), resolution_combo.getActiveElement()));
   settings.setFullscreen(display_mode_combo.getActiveElement() == 0);
   settings.setAntialias(value_at(VideoOptions::sampleCounts(), antialias_combo.getActiveElement()));
   settings.setVSync(std::clamp(vsync_combo.getActiveElement(), 0, 2));
   settings.setShowFps(fps_checkbox.isChecked());
   settings.serialize();

   if (_video_settings_handler)
   {
      _video_settings_handler();
   }
}

void MenuPageNavigator::setMonitorVideoSettingsEnabled(bool enabled)
{
   if (!enabled && _brightness_changed_connection == INVALID_CONNECTION)
   {
      return;
   }

   MenuPage& page = Menu::getInstance().getPageByName(kOptionsVideo)->get();
   MenuPageSliderItem& brightness_slider = page.getPageItem<MenuPageSliderItem>(kOptionsVideoSliderBrightness)->get();

   if (enabled)
   {
      _brightness_changed_connection =
         brightness_slider.valueChangedSignal.connect([](float value)
                                                      { GameSettings::getInstance().getVideoSettings().setBrightness(value); });
   }
   else
   {
      brightness_slider.valueChangedSignal.disconnect(_brightness_changed_connection);
      _brightness_changed_connection = INVALID_CONNECTION;
   }
}

void MenuPageNavigator::restoreVideoDefaults()
{
   // the window size isn't on the page, it stays
   GameSettings::VideoSettings& settings = GameSettings::getInstance().getVideoSettings();
   const auto width = settings.getWidth();
   const auto height = settings.getHeight();

   settings.restoreDefaults();
   settings.setWidth(width);
   settings.setHeight(height);

   deserializeVideoSettings();
}

void MenuPageNavigator::deserializeGameplaySettings()
{
   GameSettings::GameplaySettings& settings = GameSettings::getInstance().getGameplaySettings();
   MenuPage& page = Menu::getInstance().getPageByName(kOptionsGame)->get();

   MenuPageCheckBoxItem& camera_follows_checkbox = page.getPageItem<MenuPageCheckBoxItem>(kOptionsGameCheckBoxCameraFollows)->get();
   MenuPageSliderItem& shake_slider = page.getPageItem<MenuPageSliderItem>(kOptionsGameSliderShake)->get();

   camera_follows_checkbox.setChecked(settings.isCameraFollowingPlayer());
   shake_slider.setValue(settings.getCameraShakeIntensity());

   _gameplay_settings_shown = true;
}

void MenuPageNavigator::serializeGameplaySettings()
{
   MenuPage& page = Menu::getInstance().getPageByName(kOptionsGame)->get();

   MenuPageCheckBoxItem& camera_follows_checkbox = page.getPageItem<MenuPageCheckBoxItem>(kOptionsGameCheckBoxCameraFollows)->get();
   MenuPageSliderItem& shake_slider = page.getPageItem<MenuPageSliderItem>(kOptionsGameSliderShake)->get();

   GameSettings::GameplaySettings& settings = GameSettings::getInstance().getGameplaySettings();
   settings.setCameraFollowsPlayer(camera_follows_checkbox.isChecked());
   settings.setCameraShakeIntensity(shake_slider.getValue());
   settings.serialize();
}

void MenuPageNavigator::restoreGameDefaults()
{
   // the settings themselves only change on OK
   GameSettings::GameplaySettings& settings = GameSettings::getInstance().getGameplaySettings();
   const bool camera_follows = settings.isCameraFollowingPlayer();
   const float shake = settings.getCameraShakeIntensity();

   settings.restoreDefaults();
   deserializeGameplaySettings();

   settings.setCameraFollowsPlayer(camera_follows);
   settings.setCameraShakeIntensity(shake);
}

void MenuPageNavigator::deserializeControllerSettings()
{
   const auto key_map = GameSettings::getInstance().getControllerSettings().getKeyMap();
   MenuPage& page = Menu::getInstance().getPageByName(kOptionsControls)->get();

   for (const auto& field : kOptionsControlsKeyFields)
   {
      MenuPageTextEditItem& item = page.getPageItem<MenuPageTextEditItem>(std::string(field.item))->get();
      const auto it = key_map.find(field.key);
      const std::string name = it != key_map.end() ? SDL_GetKeyName(static_cast<SDL_Keycode>(it->second)) : "";

      item.setText(name);
      item.setCursorPosition(static_cast<int>(name.length()));
   }

   _controller_settings_shown = true;
}

void MenuPageNavigator::serializeControllerSettings()
{
   GameSettings::ControllerSettings& settings = GameSettings::getInstance().getControllerSettings();
   auto key_map = settings.getKeyMap();

   MenuPage& page = Menu::getInstance().getPageByName(kOptionsControls)->get();

   for (const auto& field : kOptionsControlsKeyFields)
   {
      MenuPageTextEditItem& item = page.getPageItem<MenuPageTextEditItem>(std::string(field.item))->get();
      const SDL_Keycode key = SDL_GetKeyFromName(item.getText().c_str());

      // like setKeyMapValid(false): one unreadable field keeps the whole key map
      if (key == SDLK_UNKNOWN)
      {
         qWarning("MenuPageNavigator: '%s' is no key, keyboard controls not changed", item.getText().c_str());
         return;
      }

      key_map[field.key] = static_cast<int>(key);
   }

   settings.setKeyMap(key_map);
   settings.serialize();
}

void MenuPageNavigator::restoreControlsDefaults()
{
   // the settings themselves only change on OK
   GameSettings::ControllerSettings& settings = GameSettings::getInstance().getControllerSettings();
   const auto key_map = settings.getKeyMap();
   const auto analogue_threshold = settings.getAnalogueThreshold();

   settings.restoreDefaults();
   deserializeControllerSettings();

   settings.setKeyMap(key_map);
   settings.setAnalogueThreshold(analogue_threshold);
}

void MenuPageNavigator::deserializeAudioSettings()
{
   // matches GameMenuInterfaceOptions::deserializeAudioSettings() - slider positions come from
   // SoundManager's live volume, not GameSettings directly (SoundManager itself was seeded from
   // GameSettings at startup).
   MenuPage& page = Menu::getInstance().getPageByName(kOptionsAudio)->get();

   MenuPageSliderItem& music_slider = page.getPageItem<MenuPageSliderItem>(kOptionsAudioSliderMusic)->get();
   MenuPageSliderItem& sfx_slider = page.getPageItem<MenuPageSliderItem>(kOptionsAudioSliderSfx)->get();

   music_slider.setValue(SoundManager::getInstance().getVolumeMusic());
   sfx_slider.setValue(SoundManager::getInstance().getVolumeSfx());
}

void MenuPageNavigator::setMonitorAudioSettingsEnabled(bool enabled)
{
   // matches GameMenuInterfaceOptions::setMonitorAudioSettingsEnabled() - only the sfx slider
   // gets a tick sound (audible feedback of the new sfx volume itself); the music slider doesn't.
   if (!enabled && _music_volume_changed_connection == INVALID_CONNECTION)
   {
      return;
   }

   MenuPage& page = Menu::getInstance().getPageByName(kOptionsAudio)->get();

   MenuPageSliderItem& music_slider = page.getPageItem<MenuPageSliderItem>(kOptionsAudioSliderMusic)->get();
   MenuPageSliderItem& sfx_slider = page.getPageItem<MenuPageSliderItem>(kOptionsAudioSliderSfx)->get();

   if (enabled)
   {
      _music_volume_changed_connection = music_slider.valueChangedSignal.connect([this](float value) { applyVolumeMusic(value); });
      _sfx_volume_changed_connection = sfx_slider.valueChangedSignal.connect([this](float value) { applyVolumeSfx(value); });
      _sfx_tick_connection = sfx_slider.valueChangedSignal.connect([](float) { SoundManager::getInstance().playSoundTick(); });
   }
   else
   {
      music_slider.valueChangedSignal.disconnect(_music_volume_changed_connection);
      sfx_slider.valueChangedSignal.disconnect(_sfx_volume_changed_connection);
      sfx_slider.valueChangedSignal.disconnect(_sfx_tick_connection);

      _music_volume_changed_connection = INVALID_CONNECTION;
      _sfx_volume_changed_connection = INVALID_CONNECTION;
      _sfx_tick_connection = INVALID_CONNECTION;
   }
}

void MenuPageNavigator::applyVolumeMusic(float volume)
{
   SoundManager::getInstance().setVolumeMusic(volume);
}

void MenuPageNavigator::applyVolumeSfx(float volume)
{
   SoundManager::getInstance().setVolumeSfx(volume);
}

void MenuPageNavigator::restoreAudioDefaults()
{
   // matches GameMenuInterfaceOptions::restoreAudioDefaults(): reset GameSettings, push the
   // (now-default) values into SoundManager, then re-seed the sliders' visual positions.
   GameSettings::AudioSettings& audio_settings = GameSettings::getInstance().getAudioSettings();
   audio_settings.restoreDefaults();

   SoundManager::getInstance().setVolumeMusic(audio_settings.getVolumeMusic());
   SoundManager::getInstance().setVolumeSfx(audio_settings.getVolumeSfx());

   deserializeAudioSettings();
}

void MenuPageNavigator::createGame()
{
   // matches GameMenuInterfaceCreate::createGame().
   MenuPage& page = Menu::getInstance().getPageByName(kGameCreate)->get();

   MenuPageTextEditItem& game_name_item = page.getPageItem<MenuPageTextEditItem>("lineedit_name")->get();
   MenuPageComboBoxItem& time_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_time_table")->get();
   MenuPageComboBoxItem& max_players_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_maxplayers_table")->get();
   MenuPageComboBoxItem& bot_count_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_bots_table")->get();
   MenuPageComboBoxItem& level_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_level_table")->get();
   MenuPageComboBoxItem& rounds_combo = page.getPageItem<MenuPageComboBoxItem>("combobox_rounds_table")->get();

   MenuPageCheckBoxItem& bomb_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_bomb")->get();
   MenuPageCheckBoxItem& flame_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_flame")->get();
   MenuPageCheckBoxItem& speed_up_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_speedup")->get();
   MenuPageCheckBoxItem& kick_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_kick")->get();
   MenuPageCheckBoxItem& skulls_extras_checkbox = page.getPageItem<MenuPageCheckBoxItem>("checkbox_skull")->get();

   const std::string game_name = game_name_item.getText();

   int level_index = level_combo.getActiveElement();
   std::string level_dir_name = (static_cast<size_t>(level_index) >= _sorted_level_dir_names.size()) ? _sorted_level_dir_names[0]
                                                                                                     : _sorted_level_dir_names[level_index];

   const int duration_minutes = std::atoi(time_combo.getValue().c_str());
   const int duration_seconds = duration_minutes * 60;
   const int max_players = std::atoi(max_players_combo.getValue().c_str());
   const int bot_count = std::atoi(bot_count_combo.getValue().c_str());
   const int rounds = std::atoi(rounds_combo.getValue().c_str());

   const bool extra_bombs = bomb_extras_checkbox.isChecked();
   const bool extra_flames = flame_extras_checkbox.isChecked();
   const bool extra_speed_ups = speed_up_extras_checkbox.isChecked();
   const bool extra_kicks = kick_extras_checkbox.isChecked();
   const bool extra_skulls = skulls_extras_checkbox.isChecked();

   const Constants::Dimension dimension = (max_players <= 5) ? Constants::Dimension13x11 : Constants::Dimension19x17;

   GameSettings::CreateGameSettings& create_game_settings = BombermanClient::getInstance().isSinglePlayer()
                                                               ? GameSettings::getInstance().getCreateGameSettingsSingle()
                                                               : GameSettings::getInstance().getCreateGameSettingsMulti();

   create_game_settings.setGameName(game_name);
   create_game_settings.setLevelIndex(level_index);
   create_game_settings.setRounds(rounds);
   create_game_settings.setDuration(duration_minutes);
   create_game_settings.setMaxPlayers(max_players);
   create_game_settings.setExtraBombsEnabled(extra_bombs);
   create_game_settings.setExtraFlamesEnabled(extra_flames);
   create_game_settings.setExtraKicksEnabled(extra_kicks);
   create_game_settings.setExtraSpeedUpsEnabled(extra_speed_ups);
   create_game_settings.setExtraSkullsEnabled(extra_skulls);
   create_game_settings.setDimensions(dimension);
   create_game_settings.setBotCount(bot_count);
   create_game_settings.serialize();

   BombermanClient::getInstance().createGame(
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
