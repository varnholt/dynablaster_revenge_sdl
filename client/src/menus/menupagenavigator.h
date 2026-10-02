#pragma once

#include "hosthistory.h"
#include "gamesignal.h"

#include <cstddef>
#include <functional>
#include <limits>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class MenuPage;
class PlayerInfo;

/// \brief page navigation and BombermanClient wiring for the menu system: drives the
/// login -> create game -> join game -> lounge chain and populates/reads the GAME_CREATE,
/// OPTIONS_AUDIO and LOUNGE pages (mirrors the original GameMenuWorkflow/GameMenuInterface*).
///
/// Signal<> has no auto-disconnect: every connection capturing this is disconnected again in the
/// destructor, so BombermanClient and the menu items must simply outlive this object.
class MenuPageNavigator
{
public:
   MenuPageNavigator();
   ~MenuPageNavigator();

   MenuPageNavigator(const MenuPageNavigator&) = delete;
   MenuPageNavigator& operator=(const MenuPageNavigator&) = delete;

   Signal<const std::string&> pageChangeRequestSignal;
   Signal<> quitRequestSignal;
   Signal<> addLocalPlayerRequestSignal;

   void onActionRequest(const std::string& page, const std::string& action);

   //! called instead of joining a game right away, e.g. to set up the players on this machine
   //! first; returns false to join directly. return_page is where cancelling goes back to.
   using JoinHandler = std::function<bool(int game_id, const std::string& return_page)>;
   void setJoinHandler(JoinHandler handler);

   //! mirrors GameMenuWorkflow::pageChanged() - populates GAME_CREATE's controls once the page
   //! actually becomes current. Connect to MenuDrawable::pageChangedSignal.
   void onPageChanged(const std::string& page);

private:
   void onLoginResponse(bool granted);
   void onCreateGameResponse(bool granted, int game_id, bool owner);
   void onJoinGameResponse(bool success);
   void onGameStarted();

   //! joins directly unless the join handler takes over
   void requestJoin(int game_id, const std::string& return_page);

   //! mirrors GameMenuInterfaceCreate::updateCreateGamePlayerCounts()/updateCreateGameLevelPreview()
   void updateCreateGamePlayerCounts();
   void updateCreateGameLevelPreview();

   //! mirrors GameMenuInterfaceLounge::playerInfoMapUpdated() - repopulates the lounge's player
   //! rows (nick/wins/rank/owner-icon) whenever the player set changes (join/leave/bot added).
   void onPlayerInfoMapUpdated(std::map<int, PlayerInfo*>* player_info);

   //! mirrors GameMenuInterfaceOptions::applyVolumeMusic()/applyVolumeSfx() - forwards a dragged
   //! slider's value straight to SoundManager (live volume change, not yet persisted).
   void applyVolumeMusic(float volume);
   void applyVolumeSfx(float volume);

   //! mirrors GameMenuWorkflow::messageReceived() - appends a finished chat line to the lounge's
   //! message table. Typing-in-progress notifications (finished == false) are ignored.
   void onMessageReceived(int sender_id, const std::string& message, bool finished);

   //! mirrors GameMenuInterfaceMain::deserializeLoginData() - repopulates the main menu's host
   //! combobox (from HostHistory) and nick/host text fields (from GameSettings) whenever the
   //! main menu becomes current, including once at startup (see the constructor).
   void deserializeLoginData();

   //! mirrors GameMenuInterfaceMain::updateLoginData() - captures whatever's currently typed in
   //! the nick/host fields into GameSettings + HostHistory, called before leaving the main menu.
   void updateLoginData();

   //! mirrors GameMenuInterfaceMain::deserializeVersion() - sets the main menu's bottom-left
   //! "label_version" text (its PSD opacity is 0 - always hidden until this raises its alpha).
   void deserializeVersion();

   //! mirrors GameMenuInterfaceCreate::deserializeCreateGameData() - repopulates the "game name"
   //! text field from GameSettings whenever the GAME_CREATE page becomes current.
   void deserializeCreateGameData();

   //! mirrors GameMenuInterfaceCreate::initializeCreateGameOptions()
   void initializeCreateGameOptions();

   //! mirrors GameMenuInterfaceCreate::createGame()
   void createGame();

   void setMonitorCreateGameOptionsEnabled(bool enabled);

   //! mirrors GameMenuInterfaceOptions::deserializeAudioSettings() - seeds the audio options
   //! page's sliders from SoundManager's current volume whenever OPTIONS_AUDIO becomes current.
   void deserializeAudioSettings();

   //! mirrors GameMenuInterfaceOptions::setMonitorAudioSettingsEnabled().
   void setMonitorAudioSettingsEnabled(bool enabled);

   //! mirrors GameMenuInterfaceOptions::restoreAudioDefaults() - resets GameSettings' audio
   //! volumes, re-applies them to SoundManager, and re-seeds the sliders' visual positions.
   void restoreAudioDefaults();

   //! mirrors GameMenuInterfaceLounge::playerInfoMapUpdated() - the actual row-population logic,
   //! called both on the live signal and once on first reaching LOUNGE.
   void updateLoungePlayerList(std::map<int, PlayerInfo*>* player_info);

   //! mirrors GameMenuInterfaceLounge::addLoungeMessage() - word-wraps and appends one chat line
   //! (already formatted as "nick: text" by the server) to the lounge's message table.
   void addLoungeMessage(int sender_id, const std::string& message);

   static constexpr std::size_t INVALID_CONNECTION = std::numeric_limits<std::size_t>::max();

   std::vector<std::string> _sorted_level_names;
   std::vector<std::string> _sorted_level_dir_names;
   std::unordered_set<MenuPage*> _create_game_pages_initialized;
   std::unordered_map<int, int> _player_id_to_index_map;
   HostHistory _host_history;

   //! BombermanClient connection tokens, disconnected on destruction
   std::size_t _login_response_connection = INVALID_CONNECTION;
   std::size_t _create_game_response_connection = INVALID_CONNECTION;
   std::size_t _join_game_response_connection = INVALID_CONNECTION;
   std::size_t _game_started_connection = INVALID_CONNECTION;
   std::size_t _player_info_map_updated_connection = INVALID_CONNECTION;
   std::size_t _message_received_connection = INVALID_CONNECTION;

   //! connection tokens for setMonitorCreateGameOptionsEnabled()'s connect/disconnect pair
   Signal<const std::string&>::Connection _max_players_value_changed_connection = INVALID_CONNECTION;
   Signal<const std::string&>::Connection _level_value_changed_connection = INVALID_CONNECTION;
   Signal<int>::Connection _level_element_focussed_connection = INVALID_CONNECTION;

   //! connection tokens for setMonitorAudioSettingsEnabled()'s connect/disconnect pair
   Signal<float>::Connection _music_volume_changed_connection = INVALID_CONNECTION;
   Signal<float>::Connection _sfx_volume_changed_connection = INVALID_CONNECTION;
   Signal<float>::Connection _sfx_tick_connection = INVALID_CONNECTION;

   JoinHandler _join_handler;
};
