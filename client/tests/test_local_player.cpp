// a second player on this machine joins the main client's game over its own connection to the
// embedded server, shows up in the main client's player list, moves with its own keys in a round
// and disappears again on leave.
#include "constants.h"
#include "framework/timerhandler.h"
#include "game/bombermanclient.h"
#include "game/gamestatemachine.h"
#include "game/localplayerclient.h"
#include "playerinfo.h"
#include "sdlglobaltime.h"
#include "timer.h"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include <array>
#include <cmath>
#include <functional>
#include <memory>

namespace
{
SdlGlobalTime* global_clock = nullptr;

bool waitFor(const std::function<bool()>& condition, uint64_t timeout_ms = 10000)
{
   const uint64_t start = SDL_GetTicks();
   while (!condition())
   {
      if (SDL_GetTicks() - start > timeout_ms)
      {
         return false;
      }
      Timer::update();
      global_clock->update();
      TimerHandler::Instance()->update();
      SDL_Delay(1);
   }
   return true;
}

bool check(bool condition, const char* what)
{
   if (!condition)
   {
      SDL_Log("FAILED: %s", what);
   }
   return condition;
}

bool hasPlayer(BombermanClient& client, int32_t id)
{
   return client.getPlayerInfoMap()->contains(id);
}
}  // namespace

int main(int /*argc*/, char** /*argv*/)
{
   if (!NET_Init())
   {
      SDL_Log("FAILED: SDL_net unavailable: %s", SDL_GetError());
      return 1;
   }

   // the round's frame timers run on the game clock
   SdlGlobalTime global_time;
   global_clock = &global_time;

   bool ok = true;
   {
      BombermanClient client;
      client.initialize();

      bool joined = false;
      client.loginResponseSignal.connect(
         [&](bool success)
         {
            if (success)
            {
               client.createGameAutomatic();
            }
         }
      );
      client.createGameResponseSignal.connect(
         [&](bool success, int game_id, bool)
         {
            if (success)
            {
               client.joinGame(game_id);
            }
         }
      );
      client.joinGameResponseSignal.connect([&](bool success) { joined = success; });

      client.host();
      client.loginRequest("127.0.0.1", "main");
      ok &= check(waitFor([&]() { return joined; }), "main client joins its game");

      auto second = std::make_unique<LocalPlayerClient>("127.0.0.1", "second", client.getGameId());
      int32_t second_id = -1;
      second->joinedSignal.connect([&](int32_t id) { second_id = id; });
      ok &= check(waitFor([&]() { return second_id != -1; }), "local player joins");
      ok &= check(second_id != client.getPlayerId(), "local player is a separate player");
      ok &= check(waitFor([&]() { return hasPlayer(client, second_id); }), "main client sees the local player");

      client.setLocalPlayerIds({second_id});
      ok &= check(client.isLocalPlayer(second_id) && client.isLocalPlayer(client.getPlayerId()), "both are local players");

      // a round: the local player's keys move its own player
      client.levelLoaded("");
      client.startGame(client.getGameId());
      ok &= check(waitFor([]() { return GameStateMachine::getInstance()->getState() == Constants::GameActive; }, 20000), "round starts");

      bool moved = false;
      for (const auto key : std::array<uint8_t, 4>{Constants::KeyRight, Constants::KeyDown, Constants::KeyLeft, Constants::KeyUp})
      {
         PlayerInfo* info = client.getPlayerInfo(second_id);
         const float x = info->getX();
         const float y = info->getY();
         second->setKeys(key);
         waitFor([&]() { return std::abs(info->getX() - x) + std::abs(info->getY() - y) > 0.5f; }, 1000);
         second->setKeys(0);
         if (std::abs(info->getX() - x) + std::abs(info->getY() - y) > 0.5f)
         {
            moved = true;
            break;
         }
      }
      ok &= check(moved, "local player moves with its keys");

      second->leave();
      ok &= check(waitFor([&]() { return !hasPlayer(client, second_id); }), "local player leaves");
      second.reset();

      client.leaveGameRequest();
   }

   NET_Quit();
   SDL_Log(ok ? "local player test passed" : "local player test failed");
   return ok ? 0 : 1;
}
