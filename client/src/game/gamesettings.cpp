#include "gamesettings.h"

#include <cstdint>

#define SETTINGS_FILE "data/game.ini"

// defaults

// audio
#define DEFAULT_VOLUME_SFX 0.5f
#define DEFAULT_VOLUME_MUSIC 0.5f

// video
#define DEFAULT_VIDEO_WIDTH 1024
#define DEFAULT_VIDEO_HEIGHT 576
#define DEFAULT_VIDEO_RESOLUTION 1
#define DEFAULT_VIDEO_ANTIALIAS 1
#define DEFAULT_VIDEO_FULLSCREEN false
#define DEFAULT_VIDEO_BRIGHTNESS 0.5
#define DEFAULT_VIDEO_VSYNC 1
#define DEFAULT_VIDEO_SHOWFPS false
#define DEFAULT_VIDEO_ZOOM 1
#define DEFAULT_VIDEO_BORDERLEFT 0
#define DEFAULT_VIDEO_BORDERTOP 0
#define DEFAULT_VIDEO_BORDERRIGHT 0
#define DEFAULT_VIDEO_BORDERBOTTOM 0

// control
#define KEYMAP_NOMINAL_SIZE 5
#define DEFAULT_ANALOGUE_AXIS_1 0
#define DEFAULT_ANALOGUE_AXIS_2 1
#define DEFAULT_ANALOGUE_THRESHOLD 3200

// gameplay
#define DEFAULT_GAMEPLAY_CAMERA_SHAKE_INTENSITY 1.0f
#define DEFAULT_GAMEPLAY_CAMERA_FOLLOWS_PLAYER true

GameSettings* GameSettings::s_settings = nullptr;

GameSettings::GameSettings()
{
   s_settings = this;

   // create settings instances
   _development_settings = std::make_unique<DevelopmentSettings>();
   _audio_settings = std::make_unique<AudioSettings>();
   _gameplay_settings = std::make_unique<GameplaySettings>();
   _login_settings = std::make_unique<LoginSettings>();
   _video_settings = std::make_unique<VideoSettings>();
   _video_settings_backup = std::make_unique<VideoSettings>();
   _controller_settings = std::make_unique<ControllerSettings>();
   _create_game_settings_single = std::make_unique<CreateGameSettings>();
   _create_game_settings_multi = std::make_unique<CreateGameSettings>();
   _style_settings = std::make_unique<StyleSettings>();

   _create_game_settings_single->setSinglePlayer(true);
   _create_game_settings_multi->setSinglePlayer(false);

   _settings.push_back(_development_settings.get());
   _settings.push_back(_audio_settings.get());
   _settings.push_back(_gameplay_settings.get());
   _settings.push_back(_login_settings.get());
   _settings.push_back(_video_settings.get());
   _settings.push_back(_controller_settings.get());
   _settings.push_back(_create_game_settings_single.get());
   _settings.push_back(_create_game_settings_multi.get());
   _settings.push_back(_style_settings.get());

   // deserialize settings data
   for (SettingsPrivate* settings : _settings)
      settings->deserialize();
}

GameSettings::~GameSettings() = default;

GameSettings* GameSettings::getInstance()
{
   if (!s_settings)
      new GameSettings();

   return s_settings;
}

GameSettings::SettingsPrivate::SettingsPrivate() : Settings(SETTINGS_FILE, Settings::IniFormat)
{
}

GameSettings::SettingsPrivate::~SettingsPrivate()
{
}

void GameSettings::SettingsPrivate::deserialize()
{
}

void GameSettings::SettingsPrivate::serialize()
{
}

void GameSettings::SettingsPrivate::restoreDefaults()
{
}

GameSettings::DevelopmentSettings* GameSettings::getDevelopmentSettings()
{
   return _development_settings.get();
}

GameSettings::StyleSettings* GameSettings::getStyleSettings()
{
   return _style_settings.get();
}

GameSettings::GameplaySettings* GameSettings::getGameplaySettings()
{
   return _gameplay_settings.get();
}

GameSettings::AudioSettings* GameSettings::getAudioSettings()
{
   return _audio_settings.get();
}

GameSettings::LoginSettings* GameSettings::getLoginSettings()
{
   return _login_settings.get();
}

GameSettings::VideoSettings* GameSettings::getVideoSettings()
{
   return _video_settings.get();
}

GameSettings::VideoSettings* GameSettings::getVideoSettingsBackup()
{
   return _video_settings_backup.get();
}

GameSettings::CreateGameSettings* GameSettings::getCreateGameSettingsSingle()
{
   return _create_game_settings_single.get();
}

GameSettings::CreateGameSettings* GameSettings::getCreateGameSettingsMulti()
{
   return _create_game_settings_multi.get();
}

GameSettings::ControllerSettings* GameSettings::getControllerSettings()
{
   return _controller_settings.get();
}

void GameSettings::serialize()
{
   getAudioSettings()->serialize();
   getVideoSettings()->serialize();
   getControllerSettings()->serialize();
   getGameplaySettings()->serialize();
}

GameSettings::AudioSettings::AudioSettings()
{
}

void GameSettings::AudioSettings::serialize()
{
   setValue("audio/volume_music", getVolumeMusic());
   setValue("audio/volume_sfx", getVolumeSfx());
}

void GameSettings::AudioSettings::deserialize()
{
   bool ok = false;

   float volume = 0.0f;

   volume = value("audio/volume_music").toFloat(&ok);

   setVolumeMusic(ok ? volume : DEFAULT_VOLUME_MUSIC);

   volume = value("audio/volume_sfx").toFloat(&ok);

   setVolumeSfx(ok ? volume : DEFAULT_VOLUME_SFX);

   volume = value("audio/volume_music_default").toFloat(&ok);

   setVolumeMusicDefault(ok ? volume : DEFAULT_VOLUME_MUSIC);

   volume = value("audio/volume_sfx_default").toFloat(&ok);

   setVolumeSfxDefault(ok ? volume : DEFAULT_VOLUME_SFX);

   bool shuffle_music = value("audio/shuffle_music", true).toBool();
   bool shuffle1st_track_only = value("audio/shuffle_1st_track_only", false).toBool();

   setShuffleMusicEnabled(shuffle_music);
   setShuffle1stTrackOnlyEnabled(shuffle1st_track_only);

   bool music_player_visible = value("audio/musicplayer_visible", true).toBool();
   setMusicPlayerVisibile(music_player_visible);
}

void GameSettings::AudioSettings::restoreDefaults()
{
   SettingsPrivate::restoreDefaults();

   setVolumeMusic(DEFAULT_VOLUME_MUSIC);
   setVolumeSfx(DEFAULT_VOLUME_SFX);
}

float GameSettings::AudioSettings::getVolumeMusic() const
{
   return _volume_music;
}

float GameSettings::AudioSettings::getVolumeSfx() const
{
   return _volume_sfx;
}

void GameSettings::AudioSettings::setVolumeMusic(float volume)
{
   _volume_music = volume;
}

void GameSettings::AudioSettings::setVolumeSfx(float volume)
{
   _volume_sfx = volume;
}

float GameSettings::AudioSettings::getVolumeMusicDefault() const
{
   return _volume_music_default;
}

float GameSettings::AudioSettings::getVolumeSfxDefault() const
{
   return _volume_sfx_default;
}

void GameSettings::AudioSettings::setVolumeMusicDefault(float volume)
{
   _volume_music_default = volume;
}

void GameSettings::AudioSettings::setVolumeSfxDefault(float volume)
{
   _volume_sfx_default = volume;
}

bool GameSettings::AudioSettings::isShuffleMusicEnabled() const
{
   return _shuffle_music;
}

void GameSettings::AudioSettings::setShuffleMusicEnabled(bool value)
{
   _shuffle_music = value;
}

bool GameSettings::AudioSettings::isShuffle1stTrackOnlyEnabled() const
{
   return _shuffle1st_track_only;
}

void GameSettings::AudioSettings::setShuffle1stTrackOnlyEnabled(bool value)
{
   _shuffle1st_track_only = value;
}

bool GameSettings::AudioSettings::isMusicPlayerVisibile() const
{
   return _music_player_visible;
}

void GameSettings::AudioSettings::setMusicPlayerVisibile(bool value)
{
   _music_player_visible = value;
}

GameSettings::DevelopmentSettings::DevelopmentSettings()
{
}

void GameSettings::DevelopmentSettings::deserialize()
{
   // init config
   _skip_menu = value("development/skipmenu", false).toBool();
   _show_splash = value("development/showsplash", true).toBool();
   _music_enabled = value("development/music", true).toBool();
   _level = value("development/level", "castle").toString();
   _dry_run_enabled = value("development/dryrun", false).toBool();
   _game_recording_enabled = value("development/gamerecording", false).toBool();
   _joysticks_enabled = value("development/joysticksenabled", true).toBool();

   setPageFacebook(value("development/page_facebook", "https://www.facebook.com/DynablasterRevenge").toString());

   setPagePouet(value("development/page_pouet", "http://www.pouet.net/prod.php?which=62926").toString());

   setPageHome(value("development/page_home", "https://dynablaster.titandemo.de/").toString());
}

bool GameSettings::DevelopmentSettings::isSkipMenuEnabled()
{
   return _skip_menu;
}

bool GameSettings::DevelopmentSettings::isSplashScreenEnabled()
{
   return _show_splash;
}

bool GameSettings::DevelopmentSettings::isMusicEnabled()
{
   return _music_enabled;
}

const std::string& GameSettings::DevelopmentSettings::getLevel() const
{
   return _level;
}

bool GameSettings::DevelopmentSettings::isDryRunEnabled() const
{
   return _dry_run_enabled;
}

void GameSettings::DevelopmentSettings::setDryRunEnabled(bool enabled)
{
   _dry_run_enabled = enabled;
}

bool GameSettings::DevelopmentSettings::isGameRecordingEnabled() const
{
   return _game_recording_enabled;
}

void GameSettings::DevelopmentSettings::setGameRecordingEnabled(bool value)
{
   _game_recording_enabled = value;
}

bool GameSettings::DevelopmentSettings::isJoysticksEnabled() const
{
   return _joysticks_enabled;
}

void GameSettings::DevelopmentSettings::setJoysticksEnabled(bool value)
{
   _joysticks_enabled = value;
}

std::string GameSettings::DevelopmentSettings::getPageHome() const
{
   return _page_home;
}

void GameSettings::DevelopmentSettings::setPageHome(const std::string& value)
{
   _page_home = value;
}

std::string GameSettings::DevelopmentSettings::getPageFacebook() const
{
   return _page_facebook;
}

void GameSettings::DevelopmentSettings::setPageFacebook(const std::string& value)
{
   _page_facebook = value;
}

std::string GameSettings::DevelopmentSettings::getPagePouet() const
{
   return _page_pouet;
}

void GameSettings::DevelopmentSettings::setPagePouet(const std::string& value)
{
   _page_pouet = value;
}

void GameSettings::LoginSettings::serialize()
{
   setValue("logindata/nick", getNick());
   setValue("logindata/host", getHost());
}

void GameSettings::LoginSettings::deserialize()
{
   setNick(value("logindata/nick").toString());
   setHost(value("logindata/host").toString());

   // player n nicks
   setPlayer2Nick(value("logindata/player2", "player2").toString());
   setPlayer3Nick(value("logindata/player3", "player3").toString());
   setPlayer4Nick(value("logindata/player4", "player4").toString());
   setPlayer5Nick(value("logindata/player5", "player5").toString());
   setPlayer6Nick(value("logindata/player6", "player6").toString());
   setPlayer7Nick(value("logindata/player7", "player7").toString());
   setPlayer8Nick(value("logindata/player8", "player8").toString());
   setPlayer9Nick(value("logindata/player9", "player9").toString());
   setPlayer10Nick(value("logindata/player10", "player10").toString());
}

void GameSettings::LoginSettings::setNick(const std::string& nick)
{
   _nick = nick;
}

const std::string& GameSettings::LoginSettings::getNick() const
{
   return _nick;
}

void GameSettings::LoginSettings::setHost(const std::string& host)
{
   _host = host;
}

const std::string& GameSettings::LoginSettings::getHost() const
{
   return _host;
}

std::string GameSettings::LoginSettings::getPlayer10Nick() const
{
   return _player10_nick;
}

void GameSettings::LoginSettings::setPlayer10Nick(const std::string& value)
{
   _player10_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer9Nick() const
{
   return _player9_nick;
}

void GameSettings::LoginSettings::setPlayer9Nick(const std::string& value)
{
   _player9_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer8Nick() const
{
   return _player8_nick;
}

void GameSettings::LoginSettings::setPlayer8Nick(const std::string& value)
{
   _player8_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer7Nick() const
{
   return _player7_nick;
}

void GameSettings::LoginSettings::setPlayer7Nick(const std::string& value)
{
   _player7_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer6Nick() const
{
   return _player6_nick;
}

void GameSettings::LoginSettings::setPlayer6Nick(const std::string& value)
{
   _player6_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer5Nick() const
{
   return _player5_nick;
}

void GameSettings::LoginSettings::setPlayer5Nick(const std::string& value)
{
   _player5_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer4Nick() const
{
   return _player4_nick;
}

void GameSettings::LoginSettings::setPlayer4Nick(const std::string& value)
{
   _player4_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer3Nick() const
{
   return _player3_nick;
}

void GameSettings::LoginSettings::setPlayer3Nick(const std::string& value)
{
   _player3_nick = value;
}

std::string GameSettings::LoginSettings::getPlayer2Nick() const
{
   return _player2_nick;
}

void GameSettings::LoginSettings::setPlayer2Nick(const std::string& value)
{
   _player2_nick = value;
}

GameSettings::CreateGameSettings::CreateGameSettings()
{
}

void GameSettings::CreateGameSettings::serialize()
{
   if (isSinglePlayer())
   {
      setValue("creategamesingle/gamename", getGameName());
      setValue("creategamesingle/levelindex", getLevelIndex());
      setValue("creategamesingle/rounds", getRounds());
      setValue("creategamesingle/duration", getDuration());
      setValue("creategamesingle/maxplayers", getMaxPlayers());
      setValue("creategamesingle/botcount", getBotCount());
      setValue("creategamesingle/extrabombs", isExtraBombsEnabled());
      setValue("creategamesingle/extraflames", isExtraFlamesEnabled());
      setValue("creategamesingle/extrakicks", isExtraKicksEnabled());
      setValue("creategamesingle/extraspeedups", isExtraSpeedUpsEnabled());
      setValue("creategamesingle/extraskulls", isExtraSkullsEnabled());
      setValue("creategamesingle/dimension", getDimensions());
   }
   else
   {
      setValue("creategamemulti/gamename", getGameName());
      setValue("creategamemulti/levelindex", getLevelIndex());
      setValue("creategamemulti/rounds", getRounds());
      setValue("creategamemulti/duration", getDuration());
      setValue("creategamemulti/maxplayers", getMaxPlayers());
      setValue("creategamemulti/botcount", getBotCount());
      setValue("creategamemulti/extrabombs", isExtraBombsEnabled());
      setValue("creategamemulti/extraflames", isExtraFlamesEnabled());
      setValue("creategamemulti/extrakicks", isExtraKicksEnabled());
      setValue("creategamemulti/extraspeedups", isExtraSpeedUpsEnabled());
      setValue("creategamemulti/extraskulls", isExtraSkullsEnabled());
      setValue("creategamemulti/dimension", getDimensions());
   }
}

void GameSettings::CreateGameSettings::deserialize()
{
   if (isSinglePlayer())
   {
      setGameName(value("creategamesingle/gamename", "Default").toString());
      setLevelIndex(value("creategamesingle/levelindex", 0).toInt());
      setRounds(value("creategamesingle/rounds", 1).toInt());
      setDuration(value("creategamesingle/duration", 3).toInt());
      setMaxPlayers(value("creategamesingle/maxplayers", 5).toInt());
      setBotCount(value("creategamesingle/botcount", 4).toInt());

      setExtraBombsEnabled(value("creategamesingle/extrabombs", true).toBool());
      setExtraFlamesEnabled(value("creategamesingle/extraflames", true).toBool());
      setExtraKicksEnabled(value("creategamesingle/extrakicks", false).toBool());
      setExtraSpeedUpsEnabled(value("creategamesingle/extraspeedups", false).toBool());
      setExtraSkullsEnabled(value("creategamesingle/extraskulls", false).toBool());

      setDimensions(
         static_cast<Constants::Dimension>(value("creategamesingle/dimension", static_cast<int32_t>(Constants::Dimension13x11)).toInt())
      );
   }
   else
   {
      setGameName(value("creategamemulti/gamename", "Default").toString());
      setLevelIndex(value("creategamemulti/levelindex", 0).toInt());
      setRounds(value("creategamemulti/rounds", 1).toInt());
      setDuration(value("creategamemulti/duration", 3).toInt());
      setMaxPlayers(value("creategamemulti/maxplayers", 5).toInt());
      setBotCount(value("creategamemulti/botcount", 0).toInt());

      setExtraBombsEnabled(value("creategamemulti/extrabombs", true).toBool());
      setExtraFlamesEnabled(value("creategamemulti/extraflames", true).toBool());
      setExtraKicksEnabled(value("creategamemulti/extrakicks", false).toBool());
      setExtraSpeedUpsEnabled(value("creategamemulti/extraspeedups", false).toBool());
      setExtraSkullsEnabled(value("creategamemulti/extraskulls", false).toBool());

      setDimensions(
         static_cast<Constants::Dimension>(value("creategamemulti/dimension", static_cast<int32_t>(Constants::Dimension13x11)).toInt())
      );
   }
}

void GameSettings::CreateGameSettings::setGameName(const std::string& val)
{
   _game_name = val;
}

const std::string& GameSettings::CreateGameSettings::getGameName() const
{
   return _game_name;
}

void GameSettings::CreateGameSettings::setLevelIndex(int index)
{
   _level_index = index;
}

int GameSettings::CreateGameSettings::getLevelIndex() const
{
   return _level_index;
}

void GameSettings::CreateGameSettings::setRounds(int rounds)
{
   _rounds = rounds;
}

int GameSettings::CreateGameSettings::getRounds() const
{
   return _rounds;
}

void GameSettings::CreateGameSettings::setDuration(int duration)
{
   _duration = duration;
}

int GameSettings::CreateGameSettings::getDuration() const
{
   return _duration;
}

void GameSettings::CreateGameSettings::setBotCount(int val)
{
   _bot_count = val;
}

int GameSettings::CreateGameSettings::getBotCount() const
{
   return _bot_count;
}

void GameSettings::CreateGameSettings::setMaxPlayers(int val)
{
   _max_players = val;
}

int GameSettings::CreateGameSettings::getMaxPlayers() const
{
   return _max_players;
}

void GameSettings::CreateGameSettings::setExtraBombsEnabled(bool enabled)
{
   _extra_bombs = enabled;
}

bool GameSettings::CreateGameSettings::isExtraBombsEnabled() const
{
   return _extra_bombs;
}

void GameSettings::CreateGameSettings::setExtraFlamesEnabled(bool enabled)
{
   _extra_flames = enabled;
}

bool GameSettings::CreateGameSettings::isExtraFlamesEnabled() const
{
   return _extra_flames;
}

void GameSettings::CreateGameSettings::setExtraSpeedUpsEnabled(bool enabled)
{
   _extra_speed_ups = enabled;
}

bool GameSettings::CreateGameSettings::isExtraSpeedUpsEnabled() const
{
   return _extra_speed_ups;
}

void GameSettings::CreateGameSettings::setExtraKicksEnabled(bool enabled)
{
   _extra_kicks = enabled;
}

bool GameSettings::CreateGameSettings::isExtraKicksEnabled() const
{
   return _extra_kicks;
}

void GameSettings::CreateGameSettings::setExtraSkullsEnabled(bool enabled)
{
   _extra_skulls = enabled;
}

bool GameSettings::CreateGameSettings::isExtraSkullsEnabled() const
{
   return _extra_skulls;
}

void GameSettings::CreateGameSettings::setDimensions(Constants::Dimension dimensions)
{
   _dimensions = dimensions;
}

Constants::Dimension GameSettings::CreateGameSettings::getDimensions() const
{
   return _dimensions;
}

bool GameSettings::CreateGameSettings::isSinglePlayer() const
{
   return _single_player;
}

void GameSettings::CreateGameSettings::setSinglePlayer(bool value)
{
   _single_player = value;
}

GameSettings::ControllerSettings::ControllerSettings()
    : _analogue_axis1(DEFAULT_ANALOGUE_AXIS_1), _analogue_axis2(DEFAULT_ANALOGUE_AXIS_2), _analogue_treshold(DEFAULT_ANALOGUE_THRESHOLD)
{
}

void GameSettings::ControllerSettings::serialize()
{
   if (_key_map.size() == KEYMAP_NOMINAL_SIZE)
   {
      SettingsMap serialize_map;

      for (const auto& [key, value] : _key_map)
      {
         serialize_map[std::to_string(static_cast<int>(key))] = std::to_string(value);
      }

      setValue("controller/keymap", serialize_map);
   }

   setValue("controller/analogueaxis1", getAnalogueAxis1());
   setValue("controller/analogueaxis2", getAnalogueAxis2());
   setValue("controller/analoguethreshold", getAnalogueThreshold());
}

void GameSettings::ControllerSettings::deserialize()
{
   _key_map.clear();
   initializeDefaultMap();

   SettingsMap deserialize_map = value("controller/keymap").toMap();

   bool key_ok = false;
   bool key_val_ok = false;

   for (const auto& entry : deserialize_map)
   {
      Constants::Key key = static_cast<Constants::Key>(SettingsValue(entry.first).toInt(&key_ok));
      int key_val = SettingsValue(entry.second).toInt(&key_val_ok);

      if (key_ok && key_val_ok)
      {
         _key_map[key] = key_val;
      }
   }

   setAnalogueAxis1(value("controller/analogueaxis1", DEFAULT_ANALOGUE_AXIS_1).toInt());

   setAnalogueAxis2(value("controller/analogueaxis2", DEFAULT_ANALOGUE_AXIS_2).toInt());

   setAnalogueThreshold(value("controller/analoguethreshold", DEFAULT_ANALOGUE_THRESHOLD).toInt());
}

void GameSettings::ControllerSettings::restoreDefaults()
{
   SettingsPrivate::restoreDefaults();

   _key_map.clear();
   initializeDefaultMap();

   setAnalogueAxis1(DEFAULT_ANALOGUE_AXIS_1);
   setAnalogueAxis2(DEFAULT_ANALOGUE_AXIS_2);
   setAnalogueThreshold(DEFAULT_ANALOGUE_THRESHOLD);
}

void GameSettings::ControllerSettings::setKeyMap(const std::unordered_map<Constants::Key, int>& key_map)
{
   _key_map = key_map;
}

std::unordered_map<Constants::Key, int> GameSettings::ControllerSettings::getKeyMap() const
{
   return _key_map;
}

void GameSettings::ControllerSettings::initializeDefaultMap()
{
   _key_map[Constants::KeyUp] = SDLK_UP;
   _key_map[Constants::KeyDown] = SDLK_DOWN;
   _key_map[Constants::KeyLeft] = SDLK_LEFT;
   _key_map[Constants::KeyRight] = SDLK_RIGHT;
   _key_map[Constants::KeyBomb] = SDLK_SPACE;
   _key_map[Constants::KeyZoomIn] = SDLK_RIGHTBRACKET;
   _key_map[Constants::KeyZoomOut] = SDLK_LEFTBRACKET;
   _key_map[Constants::KeyStart] = SDLK_F10;
}

SDL_Keycode GameSettings::ControllerSettings::getKey(Constants::Key key) const
{
   auto it = _key_map.find(key);
   return it != _key_map.end() ? static_cast<SDL_Keycode>(it->second) : 0;
}

SDL_Keycode GameSettings::ControllerSettings::getUpKey() const
{
   return getKey(Constants::KeyUp);
}

SDL_Keycode GameSettings::ControllerSettings::getDownKey() const
{
   return getKey(Constants::KeyDown);
}

SDL_Keycode GameSettings::ControllerSettings::getLeftKey() const
{
   return getKey(Constants::KeyLeft);
}

SDL_Keycode GameSettings::ControllerSettings::getRightKey() const
{
   return getKey(Constants::KeyRight);
}

SDL_Keycode GameSettings::ControllerSettings::getBombKey() const
{
   return getKey(Constants::KeyBomb);
}

SDL_Keycode GameSettings::ControllerSettings::getZoomInKey() const
{
   return getKey(Constants::KeyZoomIn);
}

SDL_Keycode GameSettings::ControllerSettings::getZoomOutKey() const
{
   return getKey(Constants::KeyZoomOut);
}

SDL_Keycode GameSettings::ControllerSettings::getStartKey() const
{
   return getKey(Constants::KeyStart);
}

void GameSettings::ControllerSettings::setAnalogueAxis1(int axis1)
{
   _analogue_axis1 = axis1;
}

int GameSettings::ControllerSettings::getAnalogueAxis1() const
{
   return _analogue_axis1;
}

void GameSettings::ControllerSettings::setAnalogueAxis2(int axis2)
{
   _analogue_axis2 = axis2;
}

int GameSettings::ControllerSettings::getAnalogueAxis2() const
{
   return _analogue_axis2;
}

void GameSettings::ControllerSettings::setAnalogueThreshold(int threshold)
{
   _analogue_treshold = threshold;
}

int GameSettings::ControllerSettings::getAnalogueThreshold() const
{
   return _analogue_treshold;
}

GameSettings::StyleSettings::StyleSettings()
{
}

void GameSettings::StyleSettings::deserialize()
{
   _player_colors.clear();

   SettingsMap deserialize_map = value("style/colormap").toMap();

   // if nothing was found, init with default data
   if (deserialize_map.empty())
   {
      initDefaultMap();
      serialize();
   }
   else
   {
      for (const auto& entry : deserialize_map)
      {
         try
         {
            Constants::Color key = static_cast<Constants::Color>(std::stoi(entry.first));
            Color value(entry.second);
            _player_colors.insert({key, value});
         }
         catch (const std::exception&)
         {
         }
      }
   }

   initializeIndividualColor();
}

void GameSettings::StyleSettings::initializeIndividualColor()
{
   for (const auto& entry : _player_colors)
   {
      uint32_t rgb = entry.second.rgb();

      switch (entry.first)
      {
         case Constants::ColorWhite:
            _color_white = rgb;
            break;
         case Constants::ColorBlack:
            _color_black = rgb;
            break;
         case Constants::ColorRed:
            _color_red = rgb;
            break;
         case Constants::ColorGreen:
            _color_green = rgb;
            break;
         case Constants::ColorBlue:
            _color_blue = rgb;
            break;
         case Constants::ColorGrey:
            _color_grey = rgb;
            break;
         case Constants::ColorYellow:
            _color_yellow = rgb;
            break;
         case Constants::ColorPurple:
            _color_purple = rgb;
            break;
         case Constants::ColorCyan:
            _color_cyan = rgb;
            break;
         case Constants::ColorOrange:
            _color_orange = rgb;
            break;
      }
   }
}

void GameSettings::StyleSettings::serialize()
{
   SettingsMap serialize_map;

   for (const auto& entry : _player_colors)
   {
      serialize_map[std::to_string(static_cast<int>(entry.first))] = entry.second.name();
   }

   setValue("style/colormap", serialize_map);
}

Color GameSettings::StyleSettings::getColor(Constants::Color player_color) const
{
   Color color;

   switch (player_color)
   {
      case Constants::ColorWhite:
         color = Color(_color_white);
         break;
      case Constants::ColorBlack:
         color = Color(_color_black);
         break;
      case Constants::ColorRed:
         color = Color(_color_red);
         break;
      case Constants::ColorGreen:
         color = Color(_color_green);
         break;
      case Constants::ColorBlue:
         color = Color(_color_blue);
         break;
      case Constants::ColorGrey:
         color = Color(_color_grey);
         break;
      case Constants::ColorYellow:
         color = Color(_color_yellow);
         break;
      case Constants::ColorPurple:
         color = Color(_color_purple);
         break;
      case Constants::ColorCyan:
         color = Color(_color_cyan);
         break;
      case Constants::ColorOrange:
         color = Color(_color_orange);
         break;
   }

   return color;
}

uint32_t GameSettings::StyleSettings::getRgb(Constants::Color player_color) const
{
   uint32_t rgb = Constants::ColorBlack;

   switch (player_color)
   {
      case Constants::ColorWhite:
         rgb = _color_white;
         break;
      case Constants::ColorBlack:
         rgb = _color_black;
         break;
      case Constants::ColorRed:
         rgb = _color_red;
         break;
      case Constants::ColorGreen:
         rgb = _color_green;
         break;
      case Constants::ColorBlue:
         rgb = _color_blue;
         break;
      case Constants::ColorGrey:
         rgb = _color_grey;
         break;
      case Constants::ColorYellow:
         rgb = _color_yellow;
         break;
      case Constants::ColorPurple:
         rgb = _color_purple;
         break;
      case Constants::ColorCyan:
         rgb = _color_cyan;
         break;
      case Constants::ColorOrange:
         rgb = _color_orange;
         break;
   }

   return rgb;
}

void GameSettings::StyleSettings::initDefaultMap()
{
   _player_colors.insert({Constants::ColorWhite, Color("#ffffff")});
   _player_colors.insert({Constants::ColorBlack, Color("#0e0e0e")});
   _player_colors.insert({Constants::ColorRed, Color("#ff1b1b")});
   _player_colors.insert({Constants::ColorGreen, Color("#51fb07")});
   _player_colors.insert({Constants::ColorBlue, Color("#0072ff")});
   _player_colors.insert({Constants::ColorGrey, Color("#51fb07")});
   _player_colors.insert({Constants::ColorYellow, Color("#8c8c8c")});
   _player_colors.insert({Constants::ColorPurple, Color("#fdba02")});
   _player_colors.insert({Constants::ColorCyan, Color("#00e5ff")});
   _player_colors.insert({Constants::ColorOrange, Color("#ff6000")});
}

/*
   Player 1	           #ffffff   white
   Player 2				  #0e0e0e   black
   Player 3				  #ff1b1b   red
   Player 4            #0072ff   blue
   Player 5	           #51fb07   green
   Player 6				  #8c8c8c
   Player 7				  #fdba02
   Player 8            #bd20fe
   Player 9	           #00e5ff
   Player 10           #ff6000
   Player Chat         #8bd9fe
   Player Left         #ff0000   #960707 optional
   Player Joined	     #11f105   #0b9500 optional
   Game Notification   #f6ff00
*/

GameSettings::VideoSettings::VideoSettings()
{
}

void GameSettings::VideoSettings::serialize()
{
   setValue("video/width", getWidth());
   setValue("video/height", getHeight());
   setValue("video/resolution", getResolution());
   setValue("video/antialias", getAntialias());
   setValue("video/fullscreen", isFullscreen());
   setValue("video/brightness", getBrightness());
   setValue("video/vsync", getVSync());
   setValue("video/showfps", isFpsShown());
   setValue("video/zoom", getZoom());
   setValue("video/borderleft", getBorderLeft());
   setValue("video/bordertop", getBorderTop());
   setValue("video/borderright", getBorderRight());
   setValue("video/borderbottom", getBorderBottom());
}

void GameSettings::VideoSettings::deserialize()
{
   setWidth(value("video/width", DEFAULT_VIDEO_WIDTH).toInt());
   setHeight(value("video/height", DEFAULT_VIDEO_HEIGHT).toInt());
   setResolution(value("video/resolution", DEFAULT_VIDEO_RESOLUTION).toInt());
   setAntialias(value("video/antialias", DEFAULT_VIDEO_ANTIALIAS).toInt());
   setFullscreen(value("video/fullscreen", DEFAULT_VIDEO_FULLSCREEN).toBool());
   setBrightness(value("video/brightness", DEFAULT_VIDEO_BRIGHTNESS).toFloat());
   setVSync(value("video/vsync", DEFAULT_VIDEO_VSYNC).toInt());
   setShowFps(value("video/showfps", DEFAULT_VIDEO_SHOWFPS).toBool());
   setZoom(value("video/zoom", DEFAULT_VIDEO_ZOOM).toFloat());
   setBorderLeft(value("video/borderleft", DEFAULT_VIDEO_BORDERLEFT).toInt());
   setBorderTop(value("video/bordertop", DEFAULT_VIDEO_BORDERTOP).toInt());
   setBorderRight(value("video/borderright", DEFAULT_VIDEO_BORDERRIGHT).toInt());
   setBorderBottom(value("video/borderbottom", DEFAULT_VIDEO_BORDERBOTTOM).toInt());
}

void GameSettings::VideoSettings::duplicate(GameSettings::VideoSettings* dest, GameSettings::VideoSettings* src)
{
   dest->setWidth(src->getWidth());
   dest->setHeight(src->getHeight());
   dest->setResolution(src->getResolution());
   dest->setAntialias(src->getAntialias());
   dest->setFullscreen(src->isFullscreen());
   dest->setBrightness(src->getBrightness());
   dest->setVSync(src->getVSync());
   dest->setShowFps(src->isFpsShown());
   dest->setZoom(src->getZoom());
   dest->setBorderLeft(src->getBorderLeft());
   dest->setBorderTop(src->getBorderTop());
   dest->setBorderRight(src->getBorderRight());
   dest->setBorderBottom(src->getBorderBottom());
}

void GameSettings::VideoSettings::restoreDefaults()
{
   SettingsPrivate::restoreDefaults();

   setWidth(DEFAULT_VIDEO_WIDTH);
   setHeight(DEFAULT_VIDEO_HEIGHT);
   setResolution(DEFAULT_VIDEO_RESOLUTION);
   setAntialias(DEFAULT_VIDEO_ANTIALIAS);
   setFullscreen(DEFAULT_VIDEO_FULLSCREEN);
   setBrightness(DEFAULT_VIDEO_BRIGHTNESS);
   setVSync(DEFAULT_VIDEO_VSYNC);
   setShowFps(DEFAULT_VIDEO_SHOWFPS);
   setZoom(DEFAULT_VIDEO_ZOOM);
   setBorderLeft(DEFAULT_VIDEO_BORDERLEFT);
   setBorderTop(DEFAULT_VIDEO_BORDERTOP);
   setBorderRight(DEFAULT_VIDEO_BORDERRIGHT);
   setBorderBottom(DEFAULT_VIDEO_BORDERBOTTOM);
}

void GameSettings::VideoSettings::setWidth(int width)
{
   _width = width;
}

int GameSettings::VideoSettings::getWidth() const
{
   return _width;
}

void GameSettings::VideoSettings::setHeight(int height)
{
   _height = height;
}

int GameSettings::VideoSettings::getHeight() const
{
   return _height;
}

void GameSettings::VideoSettings::setResolution(int resolution)
{
   _resolution = resolution;
}

int GameSettings::VideoSettings::getResolution() const
{
   return _resolution;
}

void GameSettings::VideoSettings::setAntialias(int samples)
{
   _antialias = samples;
}

int GameSettings::VideoSettings::getAntialias() const
{
   return _antialias;
}

void GameSettings::VideoSettings::setFullscreen(bool fullscreen)
{
   _fullscreen = fullscreen;
}

bool GameSettings::VideoSettings::isFullscreen() const
{
   return _fullscreen;
}

void GameSettings::VideoSettings::toggleFullscreen()
{
   setFullscreen(!isFullscreen());
}

void GameSettings::VideoSettings::setBrightness(float brightness)
{
   _brightness = brightness;
}

float GameSettings::VideoSettings::getBrightness() const
{
   return _brightness;
}

void GameSettings::VideoSettings::setVSync(int vsync)
{
   _v_sync = vsync;
}

int GameSettings::VideoSettings::getVSync() const
{
   return _v_sync;
}

bool GameSettings::VideoSettings::isFpsShown() const
{
   return _show_fps;
}

void GameSettings::VideoSettings::setShowFps(bool value)
{
   _show_fps = value;
}

float GameSettings::VideoSettings::getZoom() const
{
   return _zoom;
}

void GameSettings::VideoSettings::setZoom(float zoom)
{
   _zoom = zoom;
}

int GameSettings::VideoSettings::getBorderLeft() const
{
   return _border_left;
}

void GameSettings::VideoSettings::setBorderLeft(int left)
{
   _border_left = left;
}

int GameSettings::VideoSettings::getBorderTop() const
{
   return _border_top;
}

void GameSettings::VideoSettings::setBorderTop(int top)
{
   _border_top = top;
}

int GameSettings::VideoSettings::getBorderRight() const
{
   return _border_right;
}

void GameSettings::VideoSettings::setBorderRight(int right)
{
   _border_right = right;
}

int GameSettings::VideoSettings::getBorderBottom() const
{
   return _border_bottom;
}

void GameSettings::VideoSettings::setBorderBottom(int bottom)
{
   _border_bottom = bottom;
}

GameSettings::GameplaySettings::GameplaySettings()
{
}

void GameSettings::GameplaySettings::serialize()
{
   setValue("gameplay/camerashakeintensity", getCameraShakeIntensity());
   setValue("gameplay/camerafollowsplayer", isCameraFollowingPlayer());
}

void GameSettings::GameplaySettings::deserialize()
{
   setCameraShakeIntensity(value("gameplay/camerashakeintensity", DEFAULT_GAMEPLAY_CAMERA_SHAKE_INTENSITY).toFloat());

   setCameraFollowsPlayer(value("gameplay/camerafollowsplayer", DEFAULT_GAMEPLAY_CAMERA_FOLLOWS_PLAYER).toBool());
}

void GameSettings::GameplaySettings::restoreDefaults()
{
   SettingsPrivate::restoreDefaults();

   setCameraShakeIntensity(DEFAULT_GAMEPLAY_CAMERA_SHAKE_INTENSITY);
   setCameraFollowsPlayer(DEFAULT_GAMEPLAY_CAMERA_FOLLOWS_PLAYER);
}

bool GameSettings::GameplaySettings::isCameraFollowingPlayer() const
{
   return _camera_follows_player;
}

void GameSettings::GameplaySettings::setCameraFollowsPlayer(bool value)
{
   _camera_follows_player = value;
}

float GameSettings::GameplaySettings::getCameraShakeIntensity() const
{
   return _camera_shake_intensity;
}

void GameSettings::GameplaySettings::setCameraShakeIntensity(float value)
{
   _camera_shake_intensity = value;
}
