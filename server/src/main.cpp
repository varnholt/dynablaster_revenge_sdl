#include "logging.h"

// SDL
#include <SDL3_net/SDL_net.h>

// server
#include "server.h"

// shared
#include "timer.h"

#include <chrono>
#include <thread>


int main(int /*argc*/, char** /*argv*/)
{
   if (!NET_Init())
   {
      qWarning("Failed to initialize SDL_net: %s", SDL_GetError());
      return 1;
   }

   Server server;
   server.startPolling();

   // std::this_thread::sleep_for() is quantized to the OS scheduler's timer resolution - on
   // Windows this defaults to ~15.6ms regardless of the 1ms requested here (worse still,
   // requesting a finer resolution via timeBeginPeriod() is silently capped back down after a
   // few seconds for a background/console process on Windows 10 2004+). The old Qt server used
   // QTimer, backed by the OS's own high-resolution multimedia timer, and never hit this - this
   // naive sleep_for() polling loop measurably ran the 50Hz game tick at ~33Hz instead, which is
   // the real cause of the reported "players move much slower" bug. Spin-waiting via yield() for
   // the last stretch isn't subject to that quantization, so it replaces the sleep here.
   while (true)
   {
      Timer::update();

      auto next = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);
      while (std::chrono::steady_clock::now() < next)
      {
         std::this_thread::yield();
      }
   }
}
