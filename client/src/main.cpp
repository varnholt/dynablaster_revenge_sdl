// dynablaster_revenge - the game entry point. test/diagnostic modes live in the separate
// dynablaster_revenge_harness binary (main_harness.cpp), built from the same dynablaster_core library.
#include "gles3.h"
#include "glescontext.h"

#include "gldevice.h"

#include "tools/datapaths.h"

#include "sdlglobaltime.h"

#include "framework/timerhandler.h"
#include "timer.h"

#include "game/controllerinput.h"
#include "game/controllertestview.h"
#include "game/controlspage.h"
#include "game/countdowndrawable.h"
#include "game/gamedrawable.h"
#include "game/gamelogodrawable.h"
#include "game/gamemessagingdrawable.h"
#include "game/gamestatsdrawable.h"
#include "game/gamesettings.h"
#include "game/gamewindrawable.h"
#include "game/hotkeydrawable.h"
#include "game/localplayers.h"
#include "game/menucontrollerhandler.h"
#include "game/musicplayerdrawable.h"
#include "game/roundsdrawable.h"
#include "game/soundmanager.h"
#include "game/videooptions.h"
#include "game/videooutput.h"

#include "menus/bitmapfont.h"
#include "menus/fontmap.h"
#include "menus/fontpool.h"
#include "menus/gamefonts.h"
#include "menus/menu.h"
#include "menus/menudrawable.h"
#include "menus/menumousecursor.h"
#include "menus/menupagenavigator.h"

#include "game/bombermanclient.h"
#include "game/positioninterpolation.h"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include <array>
#include <functional>

#ifdef __SWITCH__
#include "platform/switchplatform.h"
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace
{

/// \brief allow-lists the editing/navigation keys MenuPageTextEditItem::keyPressed() special-cases
/// (menus/menupagetextedit.cpp) - everything else is filtered out here. Printable characters don't
/// go through this - they come from SDL_EVENT_TEXT_INPUT instead, which already gives
/// correctly-shifted/composed text.
SDL_Keycode mapEditingKey(SDL_Keycode key)
{
   switch (key)
   {
      case SDLK_BACKSPACE:
      case SDLK_DELETE:
      case SDLK_LEFT:
      case SDLK_RIGHT:
      case SDLK_HOME:
      case SDLK_END:
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
         return key;
      default:
         return SDLK_UNKNOWN;
   }
}

/// \brief the keys of GameSettings::ControllerSettings, as set on the controls options page
bool isConfiguredGameKey(SDL_Keycode key)
{
   const auto& controls = GameSettings::getInstance().getControllerSettings();
   return key == controls.getUpKey() || key == controls.getDownKey() || key == controls.getLeftKey() || key == controls.getRightKey() ||
          key == controls.getBombKey() || key == controls.getZoomInKey() || key == controls.getZoomOutKey() ||
          key == controls.getStartKey();
}

/// \brief allow-lists the keys the game side handles: the configured keymap of
/// GameSettings::ControllerSettings (arrows, space, [ ], F10 by default), the hardcoded
/// Return/Enter/Escape chat and leave keys of BombermanClient::processKeyPressed(), and the chat
/// editing keys of GameMessagingDrawable. both receive every key event, each gated on its own
/// "chat active" flag.
SDL_Keycode mapGameKey(SDL_Keycode key)
{
   switch (key)
   {
      case SDLK_UP:
      case SDLK_DOWN:
      case SDLK_LEFT:
      case SDLK_RIGHT:
      case SDLK_SPACE:
      case SDLK_LEFTBRACKET:
      case SDLK_RIGHTBRACKET:
      case SDLK_F10:
      case SDLK_RETURN:
      case SDLK_KP_ENTER:
      case SDLK_ESCAPE:
      case SDLK_TAB:
      case SDLK_BACKSPACE:
      case SDLK_DELETE:
      case SDLK_HOME:
      case SDLK_END:
         return key;
      default:
         return isConfiguredGameKey(key) ? key : SDLK_UNKNOWN;
   }
}

/// rief the keys that move the player and drop bombs, see GameSettings::ControllerSettings
bool isMovementKey(SDL_Keycode key)
{
   const auto& controls = GameSettings::getInstance().getControllerSettings();
   return key == controls.getUpKey() || key == controls.getDownKey() || key == controls.getLeftKey() || key == controls.getRightKey() ||
          key == controls.getBombKey();
}

}  // namespace

int main(int /*argc*/, char** /*argv*/)
{
#ifdef __SWITCH__
   // Before settings, audio assets or sockets; destroyed after the client/server.
   SwitchPlatform switch_platform;
   if (!switch_platform.initialize())
      return 1;
#endif
   // must run before any NET_* call (BombermanClient's own connection or an embedded Server via
   // host()); nearly everything in SDL3_net is undefined behavior before this succeeds.
   // SDL3_net builds as a stub-only lib under Emscripten (no raw sockets in a browser), so
   // NET_Init() always fails there - log it and keep going instead of aborting before the menu.
   if (!NET_Init())
   {
#ifdef __EMSCRIPTEN__
      SDL_Log("SDL_net unavailable in the browser build, networking disabled: %s", SDL_GetError());
#else
      SDL_Log("Failed to initialize SDL_net: %s", SDL_GetError());
      return 1;
#endif
   }

   // NET_Quit() runs once everything below is destroyed - the embedded server keeps polling its
   // sockets on its own thread until the BombermanClient is gone
   struct NetQuit
   {
      ~NetQuit()
      {
         NET_Quit();
      }
   } net_quit;

   // creates the embedded Server on demand via host()
   BombermanClient bomberman_client;
   bomberman_client.initialize();

   // the last windowed size, 1024x576 (16:9 like the menu's page space) by default
   const auto& video_settings = GameSettings::getInstance().getVideoSettings();
   const int window_width = video_settings.getWidth() > 0 ? video_settings.getWidth() : 1024;
   const int window_height = video_settings.getHeight() > 0 ? video_settings.getHeight() : 576;

   GlesContext context;
   if (!context.init("Dynablaster Revenge", window_width, window_height))
   {
      return 1;
   }

   // needed for SDL_EVENT_TEXT_INPUT - off by default in SDL3.
   SDL_StartTextInput(context.window());

   // shaders (loaded by GLDevice::loadShader) and textures (loaded by Image via loadtga) are
   // both plain file reads, resolved against these search paths rather than a hardcoded
   // prefix on every call site.
   DataPaths::add("data/shaders");
   DataPaths::add("data/textures");
   DataPaths::add("data/game");
   DataPaths::add("data/logo");  // GameLogoPointSprite's "pointsprite" texture

   // constructing a RenderDevice sets the global activeDevice pointer (see renderdevice.cpp) -
   // every Material/VertexBuffer/etc call below goes through this.
   GLDevice device;
   device.init();
   device.resize(context.width(), context.height());
   device.setCulling(false);  // materials normally drive this per-draw via getCulling(); not wired up yet

   SDL_Log("GL_VERSION: %s", glGetString(GL_VERSION));
   SDL_Log("GL_RENDERER: %s", glGetString(GL_RENDERER));
   SDL_Log("GL_SHADING_LANGUAGE_VERSION: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

   // animated materials and effects read this clock via GlobalTime::Instance()
   SdlGlobalTime global_time;

   registerGameFonts();

   MenuDrawable menu_drawable(device);
   menu_drawable.initializeGL();
   menu_drawable.setVisible(true);

   MenuMouseCursor menu_cursor(device);
   menu_cursor.initializeGL();
   menu_cursor.setVisible(true);

   // the menu draws its own cursor (above, MenuMouseCursor) - hide the OS cursor so the two
   // don't overlap on screen.
   SDL_HideCursor();

   // the animated main-menu logo, only visible on the main menu page (see GameLogoDrawable::pageChanged())
   GameLogoDrawable logo_drawable(device);
   logo_drawable.initializeGL();
   logo_drawable.setVisible(true);
   menu_drawable.pageChangedSignal.connect([&](const std::string& page) { logo_drawable.pageChanged(page); });

   // turns button clicks (Menu::actionRequest) into page navigation
   MenuPageNavigator navigator;
   menu_drawable.getMenu().actionRequestSignal.connect([&](const std::string& page, const std::string& action)
                                                       { navigator.onActionRequest(page, action); });
   navigator.pageChangeRequestSignal.connect([&](const std::string& page) { menu_drawable.pageChangeRequest(page); });

   // fullscreen, vsync and the fps title, at startup and whenever the video options are stored
   VideoOptions video_options(context);
   video_options.apply();
   navigator.setVideoSettingsHandler([&]() { video_options.apply(); });

   // populates GAME_CREATE's dropdowns/checkboxes once the page becomes current
   menu_drawable.pageChangedSignal.connect([&](const std::string& page) { navigator.onPageChanged(page); });

   // menu hover/click sound feedback
   menu_drawable.getMenu().layerFocussedSignal.connect([](const std::string& page, const std::string& item)
                                                       { SoundManager::getInstance().playSoundMouseOver(page, item); });
   menu_drawable.pageChangedSignal.connect([](const std::string& page) { SoundManager::getInstance().playSoundMouseClick(page); });

   // in-game rendering, starts hidden; showGame/showMenu below toggle it against the menu
   GameDrawable game_drawable(device);
   game_drawable.initializeGL();
   game_drawable.setVisible(false);

   // in-game chat, toggled together with the game drawable
   GameMessagingDrawable game_messaging_drawable(device);
   game_messaging_drawable.initializeGL();
   game_messaging_drawable.setVisible(false);

   // the HUD: clock, skull countdown, player stats and mute icons
   GameStatsDrawable game_stats_drawable(device);
   game_stats_drawable.initializeGL();
   game_stats_drawable.setVisible(false);

   // pre-round countdown overlay, drawn on top of the game scene
   CountdownDrawable countdown_drawable(device);
   countdown_drawable.initializeGL();

   // "ROUND X" slide-in text, shown at the start of each round
   RoundsDrawable rounds_drawable(device);
   rounds_drawable.initializeGL();

   // win/trophy screen, drives its own visibility off GameStateMachine's state changes
   GameWinDrawable game_win_drawable(device);
   game_win_drawable.initializeGL();

   // "now playing" notification, drives its own visibility off its fade timers
   // F1 overview of the hotkeys and extras, on top of everything
   HotkeyDrawable hotkey_drawable(device);
   hotkey_drawable.initializeGL();

   MusicPlayerDrawable music_player_drawable(device);
   music_player_drawable.initializeGL();

   // client <-> game wiring
   SoundManager::getInstance().trackChangedSignal.connect([&](const std::string& artist, const std::string& album, const std::string& track)
                                                          { music_player_drawable.showCurrentlyPlaying(artist, album, track); });
   game_drawable.level_loaded_signal.connect([&](const std::string& path) { bomberman_client.levelLoaded(path); });
   bomberman_client.loadLevelSignal.connect([&](const std::string& level) { game_drawable.loadLevel(level); });
   bomberman_client.shakeBlockSignal.connect([&](const MapItem& item) { game_drawable.shakeBlock(item); });
   bomberman_client.setPlayerPositionSignal.connect([&](int id, float x, float y, float ang)
                                                    { game_drawable.setPlayerPosition(id, x, y, ang); });
   bomberman_client.setPlayerSpeedSignal.connect([&](int id, float x, float y, float ang) { game_drawable.setPlayerSpeed(id, x, y, ang); });
   PositionInterpolation& position_interpolation = bomberman_client.getPositionInterpolation();
   position_interpolation.setPlayerPositionSignal.connect([&](int id, float x, float y, float ang)
                                                          { game_drawable.setPlayerPosition(id, x, y, ang); });
   position_interpolation.setPlayerSpeedSignal.connect([&](int id, float x, float y, float ang)
                                                       { game_drawable.setPlayerSpeed(id, x, y, ang); });
   position_interpolation.setMapItemPositionSignal.connect([&](int32_t item_id, float x, float y, float z)
                                                           { game_drawable.setMapItemPosition(item_id, x, y, z); });
   bomberman_client.removeMapItemSignal.connect([&](const MapItem& item) { position_interpolation.removeMapItem(item); });
   bomberman_client.playfieldScaleSignal.connect([&](float x, float y) { game_drawable.setPlayfieldScale(x, y); });
   bomberman_client.playfieldSizeSignal.connect([&](int width, int height) { game_drawable.setPlayfieldSize(width, height); });
   game_drawable.key_pressed_signal.connect([&](const KeyEvent& event) { bomberman_client.keyPressed(event); });
   game_drawable.key_released_signal.connect([&](const KeyEvent& event) { bomberman_client.keyReleased(event); });
   bomberman_client.createMapItemSignal.connect([&](const MapItem& item) { game_drawable.createMapItem(item); });
   bomberman_client.removeMapItemSignal.connect([&](const MapItem& item) { game_drawable.removeMapItem(item); });
   bomberman_client.destroyMapItemSignal.connect([&](const MapItem& item, float flame_count)
                                                 { game_drawable.destroyMapItem(item, flame_count); });
   bomberman_client.addPlayerSignal.connect([&](int id, const std::string& nick, Constants::Color color)
                                            { game_drawable.addPlayer(id, nick, color); });
   bomberman_client.removePlayerSignal.connect([&](int id) { game_drawable.removePlayer(id); });
   bomberman_client.extraRemovedSignal.connect([&](int x, int y, bool destroyed, Constants::ExtraType extra, int player_id)
                                               { game_drawable.extraRemoved(x, y, destroyed, extra, player_id); });
   bomberman_client.detonationSignal.connect([&](int x, int y, int up, int down, int left, int right, float intense)
                                             { game_drawable.addDetonation(x, y, up, down, left, right, intense); });
   bomberman_client.playerInfectedSignal.connect([&](int id, Constants::SkullType skull, int infector_id, int extra_x, int extra_y)
                                                 {
                                                    game_drawable.playerInfected(id, skull, infector_id, extra_x, extra_y);
                                                    game_stats_drawable.playerInfected(id, skull, infector_id, extra_x, extra_y);
                                                 });
   bomberman_client.timeChangedSignal.connect([&](int time_left, int duration) { game_stats_drawable.setGameTimeLeft(time_left, duration); });
   bomberman_client.playerIdSignal.connect([&](int id) { game_drawable.setPlayerId(id); });
   bomberman_client.countdownSignal.connect([&](int left) { countdown_drawable.countdown(left); });
   bomberman_client.messageReceivedSignal.connect([&](int sender_id, const std::string& message, bool finished)
                                                  { game_messaging_drawable.messageReceived(sender_id, message, finished); });

   // menu <-> game visibility switch
   bomberman_client.showGameSignal.connect(
      [&]()
      {
         menu_drawable.setVisible(false);
         logo_drawable.setVisible(false);
         menu_cursor.setVisible(false);
         game_drawable.setVisible(true);
         game_messaging_drawable.setVisible(true);
         game_stats_drawable.setVisible(true);
         music_player_drawable.setInGame(true);
         rounds_drawable.showGame();
      }
   );
   auto show_menu_again = [&]()
   {
      game_drawable.setVisible(false);
      game_messaging_drawable.setVisible(false);
      game_stats_drawable.setVisible(false);
      countdown_drawable.setVisible(false);
      rounds_drawable.setVisible(false);
      menu_drawable.setVisible(true);
      logo_drawable.setVisible(true);
      menu_cursor.setVisible(true);
      music_player_drawable.setInGame(false);
   };
   bomberman_client.showMenuSignal.connect(show_menu_again);
   // a finished game shows the win screen first and switches to the menu once it faded out;
   // showMenu() above (early leave, no valid game id) switches immediately
   bomberman_client.gameStoppedSignal.connect([&]() { Timer::singleShot(SHOW_WINNER_TIME_SUM, show_menu_again); });
   bomberman_client.showMainMenuSignal.connect([&]() { menu_drawable.pageChangeRequest("data/menus/mainmenu.psd"); });

#ifndef __SWITCH__
   // the Switch reads its controllers through libnx, see SwitchPlatform
   ControllerInput controller_input;
   controller_input.initialize();
   bomberman_client.rumbleSignal.connect([&](float intensity, int duration_ms) { controller_input.rumble(intensity, duration_ms); });

   MenuControllerHandler menu_controller_handler(menu_drawable, menu_cursor, controller_input);
   menu_controller_handler.initialize();
   controller_input.buttonPressedSignal.connect([&](ControllerInput::Id, ControllerInput::Button button)
                                                { menu_controller_handler.buttonPressed(button); });
   menu_drawable.pageChangedSignal.connect([&](const std::string&) { menu_controller_handler.focusDefaultItem(); });

   // the controller picture and stick calibration of the controls options
   ControllerTestView controller_test_view(controller_input);
   menu_drawable.pageChangedSignal.connect([&](const std::string& page) { controller_test_view.onPageChanged(page); });

   // further players on this machine, each on its own controller or the keyboard
   LocalPlayers local_players(controller_input, bomberman_client);
   bomberman_client.leaveGameSignal.connect([&]() { local_players.removeAll(); });

   // who plays with which device, color and name - set up before joining a game
   ControlsPage controls_page(menu_drawable, controller_input, local_players, bomberman_client);
   navigator.setBotsWaitCondition([&]() { return local_players.isJoining(); });
   navigator.setJoinHandler([&](int game_id, const std::string& return_page) { return controls_page.open(game_id, return_page); });
   menu_drawable.getMenu().actionRequestSignal.connect([&](const std::string& page, const std::string& action)
                                                       { controls_page.onActionRequest(page, action); });
   controller_input.buttonPressedSignal.connect([&](ControllerInput::Id id, ControllerInput::Button button)
                                                { controls_page.onControllerButtonPressed(id, button); });
#endif

   // movement keys steer the main player unless the keyboard belongs to another local player
   const auto keyboard_steers_main = [&](SDL_Keycode key)
   {
#ifndef __SWITCH__
      return !(local_players.isKeyboardAssigned() && isMovementKey(key));
#else
      return true;
#endif
   };

   // background music
   SoundManager::getInstance().startPlaylist();

   bool running = true;
   navigator.quitRequestSignal.connect([&running]() { running = false; });

   // the frame is drawn offscreen at the video options' resolution, then shown 16:9 with their brightness
   VideoOutput video_output(device);

   const std::array<std::reference_wrapper<Drawable>, 7> animated_drawables{
      logo_drawable, game_drawable, game_stats_drawable, countdown_drawable, rounds_drawable, game_win_drawable, music_player_drawable
   };

   // in drawing order
   const std::array<std::reference_wrapper<Drawable>, 11> painted_drawables{
      menu_drawable,
      menu_cursor,
      logo_drawable,
      game_drawable,
      game_messaging_drawable,
      game_stats_drawable,
      countdown_drawable,
      rounds_drawable,
      game_win_drawable,
      music_player_drawable,
      hotkey_drawable
   };

   // a std::function so Emscripten can drive it from requestAnimationFrame, a blocking loop
   // would freeze the browser tab
   std::function<void()> frame = [&]()
   {
#ifdef __EMSCRIPTEN__
      if (!running)
      {
         emscripten_cancel_main_loop();
         return;
      }
#endif

#ifdef __SWITCH__
      switch_platform.updateInput(game_drawable.isVisible(), game_drawable, menu_drawable, menu_cursor);
#endif
      SDL_Event event{};
      while (SDL_PollEvent(&event))
      {
#ifndef __SWITCH__
         controller_input.handleEvent(event);
#endif

         if (event.type == SDL_EVENT_QUIT)
         {
            running = false;
         }

         // Alt+Enter toggles fullscreen, regardless of menu/game state
         if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_RETURN && (event.key.mod & SDL_KMOD_ALT))
         {
            video_options.toggleFullscreen();
            continue;  // don't also forward the plain Return key to the menu/game below
         }

         // the hotkeys of the F1 overview, in the menu as well as in the game
         if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat)
         {
            bool handled = true;
            switch (event.key.key)
            {
               case SDLK_F1:
                  hotkey_drawable.toggle();
                  break;
               case SDLK_F2:
                  SoundManager::getInstance().toggleMuteMusic();
                  break;
               case SDLK_F3:
                  SoundManager::getInstance().toggleMuteSfx();
                  break;
               case SDLK_PAGEDOWN:
                  SoundManager::getInstance().playNextTrack();
                  break;
               case SDLK_PAGEUP:
                  SoundManager::getInstance().playPreviousTrack();
                  break;
               default:
                  handled = false;
                  break;
            }

            if (handled)
            {
               continue;
            }
         }

         // while the F1 overview is up, any other key or click only closes it
         if (hotkey_drawable.isVisible() && hotkey_drawable.isShown())
         {
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            {
               hotkey_drawable.setVisible(false);
               continue;
            }

            if (event.type == SDL_EVENT_TEXT_INPUT)
            {
               continue;
            }
         }

         // follow fullscreen toggles and window resizes
         if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
         {
            context.updateSize();
            device.resize(context.width(), context.height());
         }

         if (event.type == SDL_EVENT_WINDOW_RESIZED)
         {
            video_options.storeWindowSize();
         }

         // menu items live in 1920x1080 page space, not window space - the frame may be boxed
         switch (event.type)
         {
            case SDL_EVENT_MOUSE_MOTION:
            {
               int x = static_cast<int>(event.motion.x);
               int y = static_cast<int>(event.motion.y);
               video_output.toPageSpace(x, y);
               menu_drawable.mouseMoveEvent(x, y);
               menu_cursor.mouseMoveEvent(x, y);
#ifndef __SWITCH__
               menu_controller_handler.mouseMoved(x, y);
#endif
               break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            {
               int x = static_cast<int>(event.button.x);
               int y = static_cast<int>(event.button.y);
               video_output.toPageSpace(x, y);
               menu_drawable.mousePressEvent(x, y);
               menu_cursor.mousePressEvent(x, y);
#ifndef __SWITCH__
               menu_controller_handler.mousePressed();
#endif
               break;
            }
            case SDL_EVENT_MOUSE_BUTTON_UP:
               menu_drawable.mouseReleaseEvent();
               menu_cursor.mouseReleaseEvent();
               break;
            case SDL_EVENT_KEY_DOWN:
            {
               if (game_drawable.isVisible())
               {
                  const SDL_Keycode key = mapGameKey(event.key.key);
                  if (key != SDLK_UNKNOWN)
                  {
                     // both get the event, each side's "chat active" gate keeps movement and
                     // chat text entry from double-handling it
                     KeyEvent key_event(key, std::string(), event.key.repeat);
                     if (keyboard_steers_main(key))
                     {
                        game_drawable.keyPressEvent(key_event);
                     }
                     game_messaging_drawable.keyPressEvent(key_event);
                  }
               }
               else
               {
#ifndef __SWITCH__
                  if (controls_page.onKeyPressed(event.key.key))
                  {
                     break;
                  }
#endif
                  // the keyboard fields of the controls options take any key
                  if (navigator.onKeyPressed(event.key.key))
                  {
                     break;
                  }

                  const SDL_Keycode key = mapEditingKey(event.key.key);
                  if (key != SDLK_UNKNOWN)
                  {
                     KeyEvent key_event(key, std::string(), false);
                     menu_drawable.keyPressEvent(key_event);
                  }
               }
               break;
            }
            case SDL_EVENT_KEY_UP:
            {
               if (game_drawable.isVisible())
               {
                  const SDL_Keycode key = mapGameKey(event.key.key);
                  if (key != SDLK_UNKNOWN && keyboard_steers_main(key))
                  {
                     KeyEvent key_event(key, std::string(), event.key.repeat);
                     game_drawable.keyReleaseEvent(key_event);
                  }
               }
               break;
            }
            case SDL_EVENT_TEXT_INPUT:
            {
               KeyEvent key_event(SDLK_UNKNOWN, std::string(event.text.text), false);

               if (game_drawable.isVisible())
               {
                  game_messaging_drawable.keyPressEvent(key_event);
               }
               else if (!navigator.onTextInput())
               {
                  menu_drawable.keyPressEvent(key_event);
               }

               break;
            }
            default:
               break;
         }
      }

#ifndef __SWITCH__
      controller_input.update(game_drawable.isVisible(), game_drawable);
      menu_controller_handler.update();
      controls_page.update();
      controller_test_view.update();
      local_players.update(game_drawable.isVisible());
#endif

      // Timer::update() drives Server's/BombermanClient's own poll() plus every other Timer.
      Timer::update();

      // must run before the menu's paintGL(), its page cross-fade reads GlobalTime via FrameTimer
      global_time.update();

      // drives every FrameTimer's timeout() signal (e.g. PositionInterpolation's update loop)
      TimerHandler::Instance().update();

      const float time_ms = static_cast<float>(SDL_GetTicks());

      // everything animates before the frame is bound, like GameView::paintGL(): some animations
      // run GPU passes of their own. drawables animate in real seconds * 62.5
      if (menu_drawable.isVisible())
      {
         menu_drawable.animate(time_ms);
      }

      if (menu_cursor.isVisible())
      {
         menu_cursor.animate(time_ms);
      }

      for (Drawable& drawable : animated_drawables)
      {
         if (drawable.isVisible())
         {
            drawable.animate(time_ms * 0.0625f);
         }
      }

      video_output.beginFrame(context.width(), context.height());
      device.clear();

      for (Drawable& drawable : painted_drawables)
      {
         if (drawable.isVisible())
         {
            drawable.paintGL();
         }
      }

      video_output.endFrame();

      context.swap();
      video_options.frameSwapped();
   };

#ifdef __EMSCRIPTEN__
   // simulate_infinite_loop=true unwinds main()'s stack here and drives frame() off
   // requestAnimationFrame instead - the return below never actually runs in this build.
   // the C callback only takes a function pointer and a void* user data
   emscripten_set_main_loop_arg([](void* arg) { (*static_cast<std::function<void()>*>(arg))(); }, &frame, 0, true);
#else
   while (running)
   {
      frame();
   }
#endif

   return 0;
}
