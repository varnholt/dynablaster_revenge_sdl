#ifndef GAMESETTINGS_H
#define GAMESETTINGS_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL_keycode.h>

// math
#include "math/color.h"

// shared
#include <cstdint>
#include <memory>
#include "constants.h"
#include "settings.h"

class GameSettings
{
public:
   //! settings base
   class SettingsPrivate : public Settings
   {
   public:
      using SettingsMap = Settings::SettingsMap;

      //! constructor
      SettingsPrivate();

      //! destructor
      ~SettingsPrivate() override;

      //! deserialize data
      virtual void deserialize();

      //! serialize data
      virtual void serialize();

      //! restore defaults
      virtual void restoreDefaults();
   };

   //! design settings
   class StyleSettings : public SettingsPrivate
   {
   public:
      //! constructor
      StyleSettings();

      //! deserialize data
      void deserialize() override;

      //! serialize map
      void serialize() override;

      //! getter for color
      Color getColor(Constants::Color color) const;

      //! getter for rgb
      uint32_t getRgb(Constants::Color color) const;

   protected:
      //! init default map
      void initDefaultMap();

      //! init individual colors
      void initializeIndividualColor();

      //! color map
      std::map<Constants::Color, Color> _player_colors;

      uint32_t _color_white = 0;
      uint32_t _color_black = 0;
      uint32_t _color_red = 0;
      uint32_t _color_green = 0;
      uint32_t _color_blue = 0;
      uint32_t _color_grey = 0;
      uint32_t _color_yellow = 0;
      uint32_t _color_purple = 0;
      uint32_t _color_cyan = 0;
      uint32_t _color_orange = 0;
   };

   //! development settings
   class DevelopmentSettings : public SettingsPrivate
   {
   public:
      //! constructor
      DevelopmentSettings();

      //! deserialize data
      void deserialize() override;

      //! getter for skip menu flag
      bool isSkipMenuEnabled();

      //! getter for splash screen flag
      bool isSplashScreenEnabled();

      //! getter for music enabled flag
      bool isMusicEnabled();

      //! getter for development level
      const std::string& getLevel() const;

      //! getter for dryrun flag
      bool isDryRunEnabled() const;

      //! setter for dryrun flag
      void setDryRunEnabled(bool enabled);

      //! getter for game recording flag
      bool isGameRecordingEnabled() const;

      //! setter for game recording flag
      void setGameRecordingEnabled(bool value);

      //! getter for controllers enabled flag
      bool isControllersEnabled() const;

      //! setter for controllers enabled flag
      void setControllersEnabled(bool value);

      //! getter for pouet page
      std::string getPagePouet() const;

      //! setter for pouet page
      void setPagePouet(const std::string& value);

      //! getter for facebook page
      std::string getPageFacebook() const;

      //! setter for facebook page
      void setPageFacebook(const std::string& value);

      //! getter for webpage
      std::string getPageHome() const;

      //! setter for webpage
      void setPageHome(const std::string& value);

   protected:
      //! skip menu
      bool _skip_menu = false;

      //! show splash screen
      bool _show_splash = false;

      //! music enabled flag
      bool _music_enabled = false;

      //! preselected level
      std::string _level;

      //! dryrun
      bool _dry_run_enabled = false;

      //! game recording enabled
      bool _game_recording_enabled = false;

      //! controllers enabled
      bool _controllers_enabled = true;

      //! pouet page
      std::string _page_pouet;

      //! facebook page
      std::string _page_facebook;

      //! webpage
      std::string _page_home;
   };

   //! game settings
   class GameplaySettings : public SettingsPrivate
   {
   public:
      //! constructor
      GameplaySettings();

      //! serialize audio settings
      void serialize() override;

      //! deserialize audio settings
      void deserialize() override;

      //! restore audio settings
      void restoreDefaults() override;

      //! getter for shake intensity
      float getCameraShakeIntensity() const;

      //! setter for shake intensity
      void setCameraShakeIntensity(float value);

      //! getter for camera follows player flag
      bool isCameraFollowingPlayer() const;

      //! setter for camera follows player flag
      void setCameraFollowsPlayer(bool value);

   protected:
      //! camera shake intensity
      float _camera_shake_intensity = 1.0f;

      //! camera follows player
      bool _camera_follows_player = true;
   };

   //! audio settings
   class AudioSettings : public SettingsPrivate
   {
   public:
      //! constructor
      AudioSettings();

      //! serialize audio settings
      void serialize() override;

      //! deserialize audio settings
      void deserialize() override;

      //! restore audio settings
      void restoreDefaults() override;

      //! getter for music volume
      float getVolumeMusic() const;

      //! getter for sfx volume
      float getVolumeSfx() const;

      //! setter for music volume
      void setVolumeMusic(float);

      //! setter for sfx volume
      void setVolumeSfx(float);

      //! getter for default music volume
      float getVolumeMusicDefault() const;

      //! getter for default sfx volume
      float getVolumeSfxDefault() const;

      //! setter for default music volume
      void setVolumeMusicDefault(float);

      //! setter for default sfx volume
      void setVolumeSfxDefault(float);

      //! getter for shuffle music
      bool isShuffleMusicEnabled() const;

      //! setter for shuffle music
      void setShuffleMusicEnabled(bool value);

      //! getter for shuffle 1st track
      bool isShuffle1stTrackOnlyEnabled() const;

      //! setter for shuffle 1st track
      void setShuffle1stTrackOnlyEnabled(bool value);

      //! check if music player visible
      bool isMusicPlayerVisibile() const;

      //! setter for music player visible flag
      void setMusicPlayerVisibile(bool value);

   protected:
      // options

      //! sfx volume
      float _volume_sfx = 0.0f;

      //! music volume
      float _volume_music = 0.0f;

      //! sfx volume default
      float _volume_sfx_default = 0.0f;

      //! music volume default
      float _volume_music_default = 0.0f;

      //! checked if music is shuffled
      bool _shuffle_music = true;

      //! checked if 1st track is shuffled
      bool _shuffle1st_track_only = false;

      //! checked if music player shall be shown
      bool _music_player_visible = true;
   };

   class LoginSettings : public SettingsPrivate
   {
   public:
      //! serialize audio settings
      void serialize() override;

      //! deserialize audio settings
      void deserialize() override;

      //! setter for nick
      void setNick(const std::string& nick);

      //! getter for nick
      const std::string& getNick() const;

      //! setter for host
      void setHost(const std::string& host);

      //! getter for host
      const std::string& getHost() const;

      //! getter for nick of player 2
      std::string getPlayer2Nick() const;

      //! setter for nick of player 2
      void setPlayer2Nick(const std::string& value);

      //! getter for nick of player 3
      std::string getPlayer3Nick() const;

      //! setter for nick of player 3
      void setPlayer3Nick(const std::string& value);

      //! getter for nick of player 4
      std::string getPlayer4Nick() const;

      //! setter for nick of player 4
      void setPlayer4Nick(const std::string& value);

      //! getter for nick of player 5
      std::string getPlayer5Nick() const;

      //! setter for nick of player 5
      void setPlayer5Nick(const std::string& value);

      //! getter for nick of player 6
      std::string getPlayer6Nick() const;

      //! setter for nick of player 6
      void setPlayer6Nick(const std::string& value);

      //! getter for nick of player 7
      std::string getPlayer7Nick() const;

      //! setter for nick of player 7
      void setPlayer7Nick(const std::string& value);

      //! getter for nick of player 8
      std::string getPlayer8Nick() const;

      //! setter for nick of player 8
      void setPlayer8Nick(const std::string& value);

      //! getter for nick of player 9
      std::string getPlayer9Nick() const;

      //! setter for nick of player 9
      void setPlayer9Nick(const std::string& value);

      //! getter for nick of player 10
      std::string getPlayer10Nick() const;

      //! setter for nick of player 10
      void setPlayer10Nick(const std::string& value);

   protected:
      //! login nick
      std::string _nick;

      //! login host
      std::string _host;

      //! player 2 nick
      std::string _player2_nick;

      //! player 3 nick
      std::string _player3_nick;

      //! player 4 nick
      std::string _player4_nick;

      //! player 5 nick
      std::string _player5_nick;

      //! player 6 nick
      std::string _player6_nick;

      //! player 7 nick
      std::string _player7_nick;

      //! player 8 nick
      std::string _player8_nick;

      //! player 9 nick
      std::string _player9_nick;

      //! player 10 nick
      std::string _player10_nick;
   };

   class VideoSettings : public SettingsPrivate
   {
   public:
      //! constructor
      VideoSettings();

      //! serialize video settings
      void serialize() override;

      //! deserialize video settings
      void deserialize() override;

      //! backup video settings
      static void duplicate(VideoSettings* dest, VideoSettings* src);

      //! restore video settings
      void restoreDefaults() override;

      //! setter for video mode width
      void setWidth(int width);

      //! getter for video mode width
      int getWidth() const;

      //! setter for video mode height
      void setHeight(int height);

      //! getter for video mode height
      int getHeight() const;

      //! setter for video resolution
      void setResolution(int res);

      //! getter for video resolution
      int getResolution() const;

      //! setter for antialias samples
      void setAntialias(int samples);

      //! getter for antialias samples
      int getAntialias() const;

      //! setter for fullscreen mode
      void setFullscreen(bool fullscreen);

      //! getter for fullscreen mode
      bool isFullscreen() const;

      //! toggle fullscreen
      void toggleFullscreen();

      //! setter for brightness
      void setBrightness(float brightness);

      //! getter for brightness
      float getBrightness() const;

      //! check if vsync is enabled
      int getVSync() const;

      //! setter for vsync
      void setVSync(int enabled);

      //! getter for fps visibility
      bool isFpsShown() const;

      //! setter for fps visibility
      void setShowFps(bool value);

      //! get/set zoom factor (0.8..1.2  default: 1.0)
      float getZoom() const;
      void setZoom(float zoom);

      //! get/set border compensation (in pixels  default: 0)
      int getBorderLeft() const;
      void setBorderLeft(int left);
      int getBorderTop() const;
      void setBorderTop(int top);
      int getBorderRight() const;
      void setBorderRight(int right);
      int getBorderBottom() const;
      void setBorderBottom(int bottom);

   protected:
      //! window width
      int _width = -1;

      //! window height
      int _height = -1;

      //! rendering resolution (eg 1x1, 2x2, ...)
      int _resolution = 1;

      //! antialias samples
      int _antialias = 1;

      //! fullscreen flag
      bool _fullscreen = false;

      //! brightness
      float _brightness = 2.2f;

      //! vsync
      int _v_sync = 1;

      //! show fps
      bool _show_fps = true;

      //! camera zoom factor
      float _zoom = 1.0f;

      //! screen border compensation
      int _border_left = 0;
      int _border_top = 0;
      int _border_right = 0;
      int _border_bottom = 0;
   };

   class ControllerSettings : public SettingsPrivate
   {
   public:
      //! constructor
      ControllerSettings();

      //! serialize controller settings
      void serialize() override;

      //! deserialize controller settings
      void deserialize() override;

      //! restore controller settings
      void restoreDefaults() override;

      //! setter for keymap
      void setKeyMap(const std::unordered_map<Constants::Key, int>& key_map);

      //! getter for keymap
      std::unordered_map<Constants::Key, int> getKeyMap() const;

      SDL_Keycode getUpKey() const;
      SDL_Keycode getDownKey() const;
      SDL_Keycode getLeftKey() const;
      SDL_Keycode getRightKey() const;
      SDL_Keycode getBombKey() const;
      SDL_Keycode getZoomOutKey() const;
      SDL_Keycode getZoomInKey() const;
      SDL_Keycode getStartKey() const;

      //! setter for analogue sensitivity
      void setAnalogueThreshold(int);

      //! getter for analogue sensitivity
      int getAnalogueThreshold() const;

   protected:
      //! initialize default map
      void initializeDefaultMap();

      //! looks up a key in _key_map, 0 if not present
      SDL_Keycode getKey(Constants::Key key) const;

      //! keymap
      std::unordered_map<Constants::Key, int> _key_map;

      //! analogue sensitivity
      int _analogue_treshold;
   };

   class CreateGameSettings : public SettingsPrivate
   {
   public:
      //! constructor
      CreateGameSettings();

      //! serialize creategame settings
      void serialize() override;

      //! deserialize creategame settings
      void deserialize() override;

      //! setter for game name
      void setGameName(const std::string&);

      //! getter for game name
      const std::string& getGameName() const;

      //! setter for level index
      void setLevelIndex(int index);

      //! getter for level index
      int getLevelIndex() const;

      //! setter for number of rounds
      void setRounds(int);

      //! getter for number of rounds
      int getRounds() const;

      //! setter for game duration
      void setDuration(int);

      //! getter for game duration
      int getDuration() const;

      //! setter for bot count
      void setBotCount(int val);

      //! getter for bot count
      int getBotCount() const;

      //! setter for maximum player count
      void setMaxPlayers(int);

      //! getter for maximum player count
      int getMaxPlayers() const;

      //! setter for bombs extra flag
      void setExtraBombsEnabled(bool);

      //! getter for bombs extra flag
      bool isExtraBombsEnabled() const;

      //! setter for flames extra flag
      void setExtraFlamesEnabled(bool);

      //! getter for bombs extra flag
      bool isExtraFlamesEnabled() const;

      //! setter for speed-up extra flag
      void setExtraSpeedUpsEnabled(bool);

      //! setter for skulls extra flag
      void setExtraSkullsEnabled(bool);

      //! getter for bombs extra flag
      bool isExtraSpeedUpsEnabled() const;

      //! setter for kick extra flag
      void setExtraKicksEnabled(bool);

      //! getter for bombs extra flag
      bool isExtraKicksEnabled() const;

      //! getter for skulls extra flag
      bool isExtraSkullsEnabled() const;

      //! setter for dimensions
      void setDimensions(Constants::Dimension);

      //! getter for dimensions
      Constants::Dimension getDimensions() const;

      //! getter for single player flag
      bool isSinglePlayer() const;

      //! setter for single player flag
      void setSinglePlayer(bool value);

   protected:
      //! single player flag
      bool _single_player = false;

      //! game name
      std::string _game_name;

      //! level index
      int _level_index;

      //! number of rounds
      int _rounds = 0;

      //! duration
      int _duration = 0;

      //! maximum player count
      int _max_players = 0;

      //! bot count
      int _bot_count = 0;

      //! extra flag: bombs
      bool _extra_bombs = false;

      //! extra flag: flames
      bool _extra_flames = false;

      //! extra flag: speedups
      bool _extra_speed_ups = false;

      //! extra flag: kicks
      bool _extra_kicks = false;

      //! extra flag: skulls
      bool _extra_skulls = false;

      //! dimensions
      Constants::Dimension _dimensions = Constants::DimensionInvalid;
   };

   //! destructor
   virtual ~GameSettings();

   //! instance getter
   static GameSettings* getInstance();

   //! getter for development settings
   DevelopmentSettings* getDevelopmentSettings();

   //! getter for style settings
   StyleSettings* getStyleSettings();

   //! getter for gameplay settings
   GameplaySettings* getGameplaySettings();

   //! getter for audio settings
   AudioSettings* getAudioSettings();

   //! getter for login settings
   LoginSettings* getLoginSettings();

   //! getter for video settings
   VideoSettings* getVideoSettings();

   //! getter for video settings backup
   VideoSettings* getVideoSettingsBackup();

   //! getter for creategame settings
   CreateGameSettings* getCreateGameSettingsSingle();

   //! getter for creategame settings
   CreateGameSettings* getCreateGameSettingsMulti();

   //! getter for controller settings
   ControllerSettings* getControllerSettings();

   //! store all options (audio/video/controller)
   void serialize();

protected:
   //! constructor
   GameSettings();

   //! development setting
   std::unique_ptr<DevelopmentSettings> _development_settings;

   //! audio settings
   std::unique_ptr<AudioSettings> _audio_settings;

   //! gameplay settings
   std::unique_ptr<GameplaySettings> _gameplay_settings;

   //! login settings
   std::unique_ptr<LoginSettings> _login_settings;

   //! video settings
   std::unique_ptr<VideoSettings> _video_settings;

   //! video settings backup
   std::unique_ptr<VideoSettings> _video_settings_backup;

   //! controller settings
   std::unique_ptr<ControllerSettings> _controller_settings;

   //! creategame settings for single player
   std::unique_ptr<CreateGameSettings> _create_game_settings_single;

   //! creategame settings for multi player
   std::unique_ptr<CreateGameSettings> _create_game_settings_multi;

   //! design settings
   std::unique_ptr<StyleSettings> _style_settings;

   //! singleton instance
   static GameSettings* s_settings;

   //! list of settings
   std::vector<SettingsPrivate*> _settings;
};

#endif  // GAMESETTINGS_H
