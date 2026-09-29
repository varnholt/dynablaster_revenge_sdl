// SDL
#include <SDL3_net/SDL_net.h>

// server
#include "server.h"

// shared
#include "logging.h"
#include "timer.h"

// stdlib
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

   while (true)
   {
      Timer::update();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
}
