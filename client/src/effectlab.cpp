#include "effectlab.h"

#include "gles3.h"
#include "glescontext.h"
#include "screenshot.h"

#include "framework/globaltime.h"
#include "framework/timerhandler.h"
#include "gldevice.h"
#include "menus/gamefonts.h"
#include "tools/filestream.h"

#include "constants.h"
#include "game/bombermanclient.h"
#include "game/gamedrawable.h"
#include "gameinformation.h"
#include "bombmapitem.h"
#include "extramapitem.h"
#include "playerinfo.h"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <functional>
#include <map>
#include <memory>
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
      {"fuse",
       [](GameDrawable& game)
       {
          // never removed, so it lives until the process exits
          static BombMapItem bomb(LOCAL_PLAYER_ID, 2, 100, 8, 5);
          game.createMapItem(&bomb);
       }},
      {"extrareveal",
       [](GameDrawable& game)
       {
          static ExtraMapItem extra(101, Constants::ExtraFlame, 8, 5);
          game.createMapItem(&extra);
       }},
      {"extradestroy", [](GameDrawable& game) { game.extraRemoved(8, 5, true, Constants::ExtraFlame, -1); }},
   };

   return effects;
}

std::unique_ptr<PlayerInfo> makePlayerInfo(int32_t id, const std::string& nick, Constants::Color color, float x, float y)
{
   auto info = std::make_unique<PlayerInfo>();
   info->setId(id);
   info->setNick(nick);
   info->setColor(color);
   info->setPosition(x, y, 0.0f);
   return info;
}
}  // namespace

int runEffectLab(const std::string& effect, const std::string& out_dir, const std::string& level)
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

   FileStream::addPath("data/shaders");
   FileStream::addPath("data/textures");
   FileStream::addPath("data/game");
   FileStream::addPath("data/logo");

   GLDevice device;
   device.init();
   device.resize(context.width(), context.height());
   device.setCulling(false);

   FixedGlobalTime global_time;
   registerGameFonts();

   // stands in for the state the server would normally have sent
   BombermanClient client;
   client.getGames()->emplace_back(0, 2, 10, "lab", level, LOCAL_PLAYER_ID, Constants::Dimension13x11, 0, 0, 0, 0, 1, false);
   client.setGameId(0);
   client.setPlayerId(LOCAL_PLAYER_ID);

   auto local_info = makePlayerInfo(LOCAL_PLAYER_ID, "lab", Constants::ColorWhite, 6.5f, 5.5f);
   auto other_info = makePlayerInfo(OTHER_PLAYER_ID, "bot", Constants::ColorRed, 4.5f, 5.5f);
   (*client.getPlayerInfoMap())[LOCAL_PLAYER_ID] = local_info.get();
   (*client.getPlayerInfoMap())[OTHER_PLAYER_ID] = other_info.get();
   client.setCurrentPlayerInfo(local_info.get());

   GameDrawable game(&device);
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

   const int32_t last_frame = TRIGGER_FRAME + (effect == "snow" ? LONG_CAPTURE_OFFSETS.back() : CAPTURE_OFFSETS.back());

   for (int32_t frame = 0; frame <= last_frame; ++frame)
   {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
         if (event.type == SDL_EVENT_QUIT)
         {
            return 1;
         }
      }

      global_time.setFrame(frame);
      TimerHandler::Instance()->update();

      if (frame == TRIGGER_FRAME)
      {
         trigger->second(game);
      }

      const float time_ms = static_cast<float>(frame) / FPS * 1000.0f;

      glViewport(0, 0, context.width(), context.height());
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      game.animate(time_ms * 0.0625f);
      game.paintGL();

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
         if (frame == TRIGGER_FRAME + offset)
         {
            const auto ms = static_cast<int32_t>(static_cast<float>(offset) / FPS * 1000.0f + 0.5f);
            const auto path = std::format("{}/{}_{}.png", out_dir, effect, ms);
            saveScreenshot(path, context.width(), context.height());
            SDL_Log("effect lab: wrote %s", path.c_str());
         }
      }

      context.swap();
   }

   client.getPlayerInfoMap()->clear();
   client.setCurrentPlayerInfo(nullptr);

   return 0;
}
