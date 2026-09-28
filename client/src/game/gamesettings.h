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
            virtual ~SettingsPrivate();

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
            void deserialize();

            //! serialize map
            void serialize();

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
            void deserialize();

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

            //! getter for joysticks enabled flag
            bool isJoysticksEnabled() const;

            //! setter for joysticks enabled flag
            void setJoysticksEnabled(bool value);

            //! getter for pouet page
            std::string getPagePouet() const;

            //! setter for pouet page
            void setPagePouet(const std::string &value);

            //! getter for facebook page
            std::string getPageFacebook() const;

            //! setter for facebook page
            void setPageFacebook(const std::string &value);

            //! getter for webpage
            std::string getPageHome() const;

            //! setter for webpage
            void setPageHome(const std::string &value);


      protected:

            //! skip menu
            bool _skip_menu;

            //! show splash screen
            bool _show_splash;

            //! music enabled flag
            bool _music_enabled;

            //! preselected level
            std::string _level;

            //! dryrun
            bool _dry_run_enabled;

            //! game recording enabled
            bool _game_recording_enabled;

            //! joysticks enabled
            bool _joysticks_enabled;

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
            void serialize();

            //! deserialize audio settings
            void deserialize();

            //! restore audio settings
            void restoreDefaults();

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
            float _camera_shake_intensity;

            //! camera follows player
            bool _camera_follows_player;
      };


      //! audio settings
      class AudioSettings : public SettingsPrivate
      {
         public:

            //! constructor
            AudioSettings();

            //! serialize audio settings
            void serialize();

            //! deserialize audio settings
            void deserialize();

            //! restore audio settings
            void restoreDefaults();

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
            float _volume_sfx;

            //! music volume
            float _volume_music;

            //! sfx volume default
            float _volume_sfx_default;

            //! music volume default
            float _volume_music_default;

            //! checked if music is shuffled
            bool _shuffle_music;

            //! checked if 1st track is shuffled
            bool _shuffle1st_track_only;

            //! checked if music player shall be shown
            bool _music_player_visible;
      };

      class LoginSettings : public SettingsPrivate
      {
         public:

            //! serialize audio settings
            void serialize();

            //! deserialize audio settings
            void deserialize();

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
            void setPlayer2Nick(const std::string &value);

            //! getter for nick of player 3
            std::string getPlayer3Nick() const;

            //! setter for nick of player 3
            void setPlayer3Nick(const std::string &value);

            //! getter for nick of player 4
            std::string getPlayer4Nick() const;

            //! setter for nick of player 4
            void setPlayer4Nick(const std::string &value);

            //! getter for nick of player 5
            std::string getPlayer5Nick() const;

            //! setter for nick of player 5
            void setPlayer5Nick(const std::string &value);

            //! getter for nick of player 6
            std::string getPlayer6Nick() const;

            //! setter for nick of player 6
            void setPlayer6Nick(const std::string &value);

            //! getter for nick of player 7
            std::string getPlayer7Nick() const;

            //! setter for nick of player 7
            void setPlayer7Nick(const std::string &value);

            //! getter for nick of player 8
            std::string getPlayer8Nick() const;

            //! setter for nick of player 8
            void setPlayer8Nick(const std::string &value);

            //! getter for nick of player 9
            std::string getPlayer9Nick() const;

            //! setter for nick of player 9
            void setPlayer9Nick(const std::string &value);

            //! getter for nick of player 10
            std::string getPlayer10Nick() const;

            //! setter for nick of player 10
            void setPlayer10Nick(const std::string &value);


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
            void serialize();

            //! deserialize video settings
            void deserialize();

            //! backup video settings
            static void duplicate(VideoSettings* dest, VideoSettings* src);

            //! restore video settings
            void restoreDefaults();

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
            int _width;

            //! window height
            int _height;

            //! rendering resolution (eg 1x1, 2x2, ...)
            int _resolution;

            //! antialias samples
            int _antialias;

            //! fullscreen flag
            bool _fullscreen;

            //! brightness
            float _brightness;

            //! vsync
            int _v_sync;

            //! show fps
            bool _show_fps;

            //! camera zoom factor
            float _zoom;

            //! screen border compensation
            int _border_left;
            int _border_top;
            int _border_right;
            int _border_bottom;
      };


      class ControllerSettings : public SettingsPrivate
      {
         public:

            //! constructor
            ControllerSettings();

            //! serialize controller settings
            void serialize();

            //! deserialize controller settings
            void deserialize();

            //! restore controller settings
            void restoreDefaults();

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

            //! setter for analogue axis 1
            void setAnalogueAxis1(int axis1);

            //! getter for analogue axis 1
            int getAnalogueAxis1() const;

            //! setter for analogue axis 2
            void setAnalogueAxis2(int axis2);

            //! getter for analogue axis 2
            int getAnalogueAxis2() const;

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

            //! analogue axis 1
            int _analogue_axis1;

            //! analogue axis 2
            int _analogue_axis2;

            //! analogue sensitivity
            int _analogue_treshold;
      };


      class CreateGameSettings : public SettingsPrivate
      {
         public:

            //! constructor
            CreateGameSettings();

            //! serialize creategame settings
            void serialize();

            //! deserialize creategame settings
            void deserialize();

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
            bool _single_player;

            //! game name
            std::string _game_name;

            //! level index
            int _level_index;

            //! number of rounds
            int _rounds;

            //! duration
            int _duration;

            //! maximum player count
            int _max_players;

            //! bot count
            int _bot_count;

            //! extra flag: bombs
            bool _extra_bombs;

            //! extra flag: flames
            bool _extra_flames;

            //! extra flag: speedups
            bool _extra_speed_ups;

            //! extra flag: kicks
            bool _extra_kicks;

            //! extra flag: skulls
            bool _extra_skulls;

            //! dimensions
            Constants::Dimension _dimensions;
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
      DevelopmentSettings* _development_settings;

      //! audio settings
      AudioSettings* _audio_settings;

      //! gameplay settings
      GameplaySettings* _gameplay_settings;

      //! login settings
      LoginSettings* _login_settings;

      //! video settings
      VideoSettings* _video_settings;

      //! video settings backup
      VideoSettings* _video_settings_backup;

      //! controller settings
      ControllerSettings* _controller_settings;

      //! creategame settings for single player
      CreateGameSettings* _create_game_settings_single;

      //! creategame settings for multi player
      CreateGameSettings* _create_game_settings_multi;

      //! design settings
      StyleSettings* _style_settings;

      //! singleton instance
      static GameSettings* s_settings;

      //! list of settings
      std::vector<SettingsPrivate*> _settings;
};

#endif // GAMESETTINGS_H

