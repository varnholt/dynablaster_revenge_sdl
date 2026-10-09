#include "effectlab.h"

#include "gles3.h"
#include "glescontext.h"
#include "screenshot.h"

#include "framework/globaltime.h"
#include "framework/timerhandler.h"
#include "gldevice.h"
#include "menus/gamefonts.h"
#include "tools/datapaths.h"

#include "bombmapitem.h"
#include "constants.h"
#include "extramapitem.h"
#include "game/bombermanclient.h"
#include "game/gamedrawable.h"
#include "game/gamesettings.h"
#include "game/videooutput.h"
#include "gameinformation.h"
#include "playerinfo.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <vector>

namespace
{
constexpr int32_t WIDTH = 1024;
constexpr int32_t HEIGHT = 576;
constexpr float FPS = 60.0f;
constexpr int32_t TRIGGER_FRAME = 60;
constexpr std::array<int32_t, 6> CAPTURE_OFFSETS = {6, 30, 60, 120, 180, 300};
// the extra animations are short, sample them more densely too
constexpr std::array<int32_t, 4> SHORT_CAPTURE_OFFSETS = {3, 9, 15, 21};
// snow only shows once flakes have fallen through and respawned after the countdown
constexpr std::array<int32_t, 3> LONG_CAPTURE_OFFSETS = {600, 900, 1200};

// live view: the sealed exit portal opens after a while
constexpr float EXIT_SEALED_SECONDS = 3.0f;

constexpr int32_t LOCAL_PLAYER_ID = 0;
constexpr int32_t OTHER_PLAYER_ID = 1;

class FixedGlobalTime : public GlobalTime
{
public:
   void setFrame(int32_t frame)
   {
      _time = static_cast<float>(frame) / FPS;
   }

   float getTime() const override
   {
      return _time;
   }

private:
   float _time = 0.0f;
};

using Trigger = std::function<void(GameDrawable&)>;

const std::map<std::string, Trigger>& triggers()
{
   static const std::map<std::string, Trigger> effects = {
      {"baseline", [](GameDrawable&) {}},
      {"snow", [](GameDrawable&) {}},
      {"mushroom", [](GameDrawable& game) { game.playerInfected(LOCAL_PLAYER_ID, Constants::SkullMushroom, -1, 6, 5); }},
      {"invisible", [](GameDrawable& game) { game.playerInfected(LOCAL_PLAYER_ID, Constants::SkullInvisible, -1, 6, 5); }},
      {"invincible", [](GameDrawable& game) { game.playerInfected(LOCAL_PLAYER_ID, Constants::SkullInvincible, -1, 6, 5); }},
      {"infected", [](GameDrawable& game) { game.playerInfected(LOCAL_PLAYER_ID, Constants::SkullSlow, -1, 6, 5); }},
      {"startalers", [](GameDrawable& game) { game.extraRemoved(6, 5, false, Constants::ExtraBomb, LOCAL_PLAYER_ID); }},
      {"death", [](GameDrawable& game) { game.removePlayer(LOCAL_PLAYER_ID); }},
      {"detonation", [](GameDrawable& game) { game.addDetonation(6, 5, 2, 2, 3, 3, 1.0f); }},
      {"fuse",
       [](GameDrawable& game)
       {
          // never removed, so it lives until the process exits
          static BombMapItem bomb(LOCAL_PLAYER_ID, 2, 100, 8, 5);
          game.createMapItem(bomb);
       }},
      {"extrareveal",
       [](GameDrawable& game)
       {
          static ExtraMapItem extra(101, Constants::ExtraFlame, 8, 5);
          game.createMapItem(extra);
       }},
      {"exitsealed",
       [](GameDrawable& game)
       {
          static ExtraMapItem exit(102, Constants::ExtraExit, 8, 5);
          game.setStoryEnemiesLeft(3);
          game.createMapItem(exit);
       }},
      {"exitopen",
       [](GameDrawable& game)
       {
          static ExtraMapItem exit(102, Constants::ExtraExit, 8, 5);
          game.setStoryEnemiesLeft(3);
          game.createMapItem(exit);
          game.setStoryEnemiesLeft(0);
       }},
      {"extradestroy", [](GameDrawable& game) { game.extraRemoved(8, 5, true, Constants::ExtraFlame, -1); }},
   };

   return effects;
}

void addPlayerInfo(BombermanClient& client, int32_t id, const std::string& nick, Constants::Color color, float x, float y)
{
   PlayerInfo& info = client.addPlayerInfo(id);
   info.setId(id);
   info.setNick(nick);
   info.setColor(color);
   info.setPosition(x, y, 0.0f);
}
}  // namespace

int runEffectLab(const std::string& effect, const std::string& out_dir, const std::string& level, bool live)
{
   const auto trigger = triggers().find(effect);
   if (trigger == triggers().end())
   {
      SDL_Log("--effect=%s: unknown effect", effect.c_str());
      return 1;
   }

   std::filesystem::create_directories(out_dir);

   GlesContext context;
   if (!context.init("dynablaster effect lab", WIDTH, HEIGHT))
   {
      return 1;
   }

   if (live)
   {
      SDL_SetWindowFullscreen(context.window(), true);
      SDL_SyncWindow(context.window());
      context.updateSize();
   }

   DataPaths::add("data/shaders");
   DataPaths::add("data/textures");
   DataPaths::add("data/game");
   DataPaths::add("data/logo");

   GLDevice device;
   device.init();
   device.resize(context.width(), context.height());
   device.setCulling(false);

   FixedGlobalTime global_time;
   registerGameFonts();

   // stands in for the state the server would normally have sent
   BombermanClient client;
   client.getGames().emplace_back(0, 2, 10, "lab", level, LOCAL_PLAYER_ID, Constants::Dimension13x11, 0, 0, 0, 0, 1, false);
   client.setGameId(0);
   client.setPlayerId(LOCAL_PLAYER_ID);

   addPlayerInfo(client, LOCAL_PLAYER_ID, "lab", Constants::ColorWhite, 6.5f, 5.5f);
   addPlayerInfo(client, OTHER_PLAYER_ID, "bot", Constants::ColorRed, 4.5f, 5.5f);
   client.setCurrentPlayerId(LOCAL_PLAYER_ID);

   GameDrawable game(device);
   game.initializeGL();
   game.setVisible(true);
   game.setPlayerNamesEnabled(false);
   game.setPlayfieldSize(13, 11);
   game.setPlayfieldScale(1.0f, 1.0f);
   game.loadLevel(level);
   game.setPlayerId(LOCAL_PLAYER_ID);
   game.addPlayer(LOCAL_PLAYER_ID, "lab", Constants::ColorWhite);
   game.addPlayer(OTHER_PLAYER_ID, "bot", Constants::ColorRed);
   game.setPlayerPosition(LOCAL_PLAYER_ID, 6.5f, 5.5f, 0.0f);
   game.setPlayerPosition(OTHER_PLAYER_ID, 4.5f, 5.5f, 0.0f);

   // captures mustn't depend on the local video options
   auto& video_settings = GameSettings::getInstance().getVideoSettings();
   video_settings.setResolution(1);
   video_settings.setAntialias(1);
   video_settings.setBrightness(0.5f);
   VideoOutput video_output(device);

   const int32_t last_frame = TRIGGER_FRAME + (effect == "snow" ? LONG_CAPTURE_OFFSETS.back() : CAPTURE_OFFSETS.back());

   const uint64_t start_ms = SDL_GetTicks();
   bool triggered = false;

   for (int32_t frame = 0; live || frame <= last_frame; ++frame)
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         if (event.type == SDL_EVENT_QUIT || (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE))
         {
            return live ? 0 : 1;
         }

         if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
         {
            context.updateSize();
            device.resize(context.width(), context.height());
         }
      }

      if (live)
      {
         // real time; the trigger frame still comes first, so frames may get skipped
         frame = std::max(frame, static_cast<int32_t>(static_cast<float>(SDL_GetTicks() - start_ms) * FPS / 1000.0f));

         if (effect.starts_with("exit") && triggered)
         {
            const float seconds = static_cast<float>(frame - TRIGGER_FRAME) / FPS;
            game.setStoryEnemiesLeft(seconds < EXIT_SEALED_SECONDS ? 1 : 0);
         }
      }

      global_time.setFrame(frame);
      TimerHandler::Instance().update();

      if (!triggered && frame >= TRIGGER_FRAME)
      {
         trigger->second(game);
         triggered = true;
      }

      const float time_ms = static_cast<float>(frame) / FPS * 1000.0f;

      // like the game: animate first, draw into the offscreen frame, present it
      game.animate(time_ms * 0.0625f);

      video_output.beginFrame(context.width(), context.height());
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      game.paintGL();

      video_output.endFrame();

      std::vector<int32_t> offsets(CAPTURE_OFFSETS.begin(), CAPTURE_OFFSETS.end());
      if (effect.starts_with("extra"))
      {
         offsets.insert(offsets.end(), SHORT_CAPTURE_OFFSETS.begin(), SHORT_CAPTURE_OFFSETS.end());
      }
      else if (effect == "snow")
      {
         offsets.insert(offsets.end(), LONG_CAPTURE_OFFSETS.begin(), LONG_CAPTURE_OFFSETS.end());
      }

      for (const auto offset : offsets)
      {
         if (!live && frame == TRIGGER_FRAME + offset)
         {
            const auto ms = static_cast<int32_t>(static_cast<float>(offset) / FPS * 1000.0f + 0.5f);
            const auto path = std::format("{}/{}_{}.png", out_dir, effect, ms);
            saveScreenshot(path, context.width(), context.height());
            SDL_Log("effect lab: wrote %s", path.c_str());
         }
      }

      context.swap();
   }

   return 0;
}
