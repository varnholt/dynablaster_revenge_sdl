// SDL
#include <SDL3_net/SDL_net.h>

// server
#include "server.h"

// shared
#include "log.h"
#include "timer.h"

// stdlib
#include <chrono>
#include <format>
#include <thread>

int main(int /*argc*/, char** /*argv*/)
{
   if (!NET_Init())
   {
      Log::warning(std::format("Failed to initialize SDL_net: {}", SDL_GetError()));
      return 1;
   }

   Server server;

   // the port is taken, e.g. by another server; nobody could ever connect to this one
   if (!server.isListening())
   {
      NET_Quit();
      return 1;
   }

   server.startPolling();

   while (true)
   {
      Timer::update();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
}
