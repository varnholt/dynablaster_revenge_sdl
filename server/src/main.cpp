// SDL
#include <SDL3_net/SDL_net.h>

// server
#include "server.h"
#include "serveroptions.h"

// shared
#include "log.h"
#include "timer.h"

// stdlib
#include <chrono>
#include <format>
#include <thread>
#include <utility>

int main(int argc, char** argv)
{
   auto options = ServerOptions::parse(argc, argv);
   if (!options)
   {
      Log::warning(options.error());
      return 1;
   }
   if (options->show_help)
   {
      Log::info(ServerOptions::usage);
      return 0;
   }
   if (!NET_Init())
   {
      Log::warning(std::format("Failed to initialize SDL_net: {}", SDL_GetError()));
      return 1;
   }

   // Declared before Server so its sockets are destroyed before SDL_net shuts down.
   struct NetQuit
   {
      ~NetQuit()
      {
         NET_Quit();
      }
   } net_quit;

   Server server(true, options->websocket_port, std::move(options->websocket_origins));

   // the port is taken, e.g. by another server; nobody could ever connect to this one
   if (!server.isListening())
   {
      return 1;
   }

   server.startPolling();

   while (true)
   {
      Timer::update();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
   }
}
