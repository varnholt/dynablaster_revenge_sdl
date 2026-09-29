#pragma once

#include <cstdint>

// server
inline constexpr const char* SERVER_CONFIG_FILE_SERVER = "data/server.ini";
inline constexpr float SERVER_SPEED = 0.05f;
inline constexpr int32_t SERVER_HEARTBEAT_IN_HZ = 50;
inline constexpr int32_t SERVER_PORT = 6300;
inline constexpr float SERVER_MOVE_EPSILON = 0.3f;
inline constexpr int32_t SERVER_PREPARATION_SYNC_TIME = 2;
inline constexpr int32_t SERVER_PREPARATION_TIME = 4;  // 3, 2, 1, go
inline constexpr int32_t SERVER_FINISHING_TIME = 1500;
inline constexpr int32_t SERVER_BOMB_TICKTIME_DEFAULT = 3000;
inline constexpr int32_t SERVER_BOMB_NEIGHBOUR_DELAY = 100;
inline constexpr int32_t SERVER_IMMUNE_FIELD_DELAY = 1000;
inline constexpr int32_t SERVER_PLAYER_SYNC_MAX_TIME = 20000;
inline constexpr int32_t SERVER_SHAKE_CHECK_INTERVAL = 250;
inline constexpr int32_t SERVER_DEFAULT_BOMBCOUNT = 1;
inline constexpr int32_t SERVER_DEFAULT_FLAMECOUNT = 2;
inline constexpr int32_t SERVER_DEFAULT_SPEEDUPS = 0;
inline constexpr float SERVER_DEFAULT_SPEED = 1.0f;
inline constexpr float SERVER_SPEEDUP_INCREMENT = 0.15f;
inline constexpr int32_t SERVER_MAX_SPEEDUPS = 5;
inline constexpr int32_t SERVER_SKULL_DURATION = 20000;
inline constexpr int32_t SERVER_SKULL_BOMBCOUNT_MIN = 1;
inline constexpr int32_t SERVER_SKULL_BOMBCOUNT_MAX = 10;
inline constexpr int32_t SERVER_SKULL_FLAMECOUNT_MIN = 1;
inline constexpr int32_t SERVER_SKULL_FLAMECOUNT_MAX = 10;
inline constexpr float SERVER_SKULL_SPEED_MIN = SERVER_DEFAULT_SPEED * 0.5f;
inline constexpr float SERVER_SKULL_SPEED_MAX = SERVER_DEFAULT_SPEED + (SERVER_SPEEDUP_INCREMENT * 10.0f);
inline constexpr float SERVER_WEIGHT_BOMBS = 30.0f;
inline constexpr float SERVER_WEIGHT_FLAMES = 30.0f;
inline constexpr float SERVER_WEIGHT_KICKS = 10.0f;
inline constexpr float SERVER_WEIGHT_SKULLS = 15.0f;
inline constexpr float SERVER_WEIGHT_SPEEDUPS = 15.0f;
inline constexpr int32_t SERVER_MAX_REMAINING_TIME = 10;
inline constexpr int32_t SERVER_SPECTATOR_DELAY = 4000;
inline constexpr float SERVER_KICK_PLAYER_DISTANCE = 0.85f;
inline constexpr int32_t SERVER_SPAWN_INTERVAL = 15000;

// client
inline constexpr int32_t CLIENT_MESSAGE_TEXT_MAXIMUM = 80;
inline constexpr int32_t SHOW_WINNER_FADE_IN_TIME = 500;
inline constexpr int32_t SHOW_WINNER_DISPLAY_TIME = 4000;
inline constexpr int32_t SHOW_WINNER_FADE_OUT_TIME = 500;
inline constexpr int32_t SHOW_WINNER_ADDITIONAL_TIME = 2000;
inline constexpr int32_t SHOW_WINNER_SHOW_MENU_TIME = 1000;
inline constexpr int32_t SHOW_WINNER_TIME_SUM = SHOW_WINNER_FADE_IN_TIME + SHOW_WINNER_DISPLAY_TIME + SHOW_WINNER_FADE_OUT_TIME +
                                                SHOW_WINNER_ADDITIONAL_TIME + SHOW_WINNER_SHOW_MENU_TIME;
inline constexpr float ITEM_INTERPOLATION_EPS = 0.3f;

class Constants
{
public:
   //! key enum
   enum Key
   {
      KeyUp = 0x01,
      KeyDown = 0x02,
      KeyLeft = 0x04,
      KeyRight = 0x08,
      KeyBomb = 0x10,
      KeyZoomIn = 0x20,
      KeyZoomOut = 0x40,
      KeyStart = 0x80
   };

   //! directions
   enum Direction
   {
      DirectionUnknown = -1,
      DirectionUp,
      DirectionDown,
      DirectionLeft,
      DirectionRight
   };

   //! player color
   enum Color
   {
      ColorWhite = 1,
      ColorBlack = 2,
      ColorRed = 3,
      ColorGreen = 4,
      ColorBlue = 5,
      ColorGrey = 6,
      ColorYellow = 7,
      ColorPurple = 8,
      ColorCyan = 9,
      ColorOrange = 10
   };

   //! game state
   enum GameState
   {
      GameStopped = 1,
      GamePreparing = 2,
      GameActive = 3,
      GameFinishing = 4
   };

   //! the extra's type
   enum ExtraType
   {
      ExtraBomb = 0x1,
      ExtraFlame = 0x2,
      ExtraSpeedup = 0x4,
      ExtraKick = 0x8,
      ExtraSkull = 0x10
   };

   //! supported skull types
   enum SkullType
   {
      SkullAutofire = 0,
      SkullMinimumBomb = 1,
      SkullKeyboardInvert = 2,
      SkullMushroom = 3,
      SkullInvisible = 4,
      SkullInvincible = 5,
      SkullMaximumBomb = 6,
      SkullSlow = 7,
      SkullFast = 8,
      SkullNoBomb = 9,
      SkullReset = 10
   };

   //! playfield dimensions
   enum Dimension
   {
      DimensionInvalid = 0,
      Dimension13x11 = 143,
      Dimension19x17 = 323,
      Dimension25x21 = 525
   };

   //! help severity
   enum HelpSeverity
   {
      HelpSeverityNotification,
      HelpSeverityError
   };

   //! help location
   enum HelpLocation
   {
      HelpLocationTopLeft,
      HelpLocationTopRight,
      HelpLocationBottomLeft,
      HelpLocationButtomRight
   };

   //! error type
   enum ErrorType
   {
      ErrorDefault,
      ErrorSyncTimeout
   };

   //! game mode
   enum GameMode
   {
      GameModeSinglePlayer,
      GameModeMultiPlayer
   };
};
