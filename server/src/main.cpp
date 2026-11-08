// SDL
#include <SDL3_net/SDL_net.h>

// server
#include "server.h"

// shared
#include "log.h"
#include "timer.h"

// stdlib
#include <chrono>
#include <charconv>
#include <format>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

int main(int argc, char** argv)
{
   uint16_t websocket_port = 0;
   std::vector<std::string> origins;
   for (int i = 1; i < argc; ++i)
   {
      const std::string_view option(argv[i]);
      if (option == "--help")
      {
         Log::info("Usage: dynablaster_server [--websocket-port PORT --origin URL [--origin URL ...]]");
         return 0;
      }
      if ((option != "--websocket-port" && option != "--origin") || i + 1 == argc)
      {
         Log::warning("Expected --websocket-port PORT or --origin URL; use --help for usage");
         return 1;
      }
      const std::string_view value(argv[++i]);
      if (option == "--origin")
      {
         if ((!value.starts_with("http://") && !value.starts_with("https://")) || value.find_first_of("\r\n") != std::string_view::npos)
         {
            Log::warning("An origin must be an http:// or https:// page origin");
            return 1;
         }
         origins.emplace_back(value);
      }
      else
      {
         unsigned port = 0;
         const auto result = std::from_chars(value.data(), value.data() + value.size(), port);
         if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || port == 0 || port > 65535 || port == SERVER_PORT)
         {
            Log::warning("WebSocket port must be 1..65535 and differ from the game TCP port");
            return 1;
         }
         websocket_port = static_cast<uint16_t>(port);
      }
   }
   if (websocket_port && origins.empty())
   {
      Log::warning("--websocket-port requires at least one --origin URL");
      return 1;
   }
   if (!websocket_port && !origins.empty())
   {
      Log::warning("--origin requires --websocket-port PORT");
      return 1;
   }
   if (!NET_Init())
   {
      Log::warning(std::format("Failed to initialize SDL_net: {}", SDL_GetError()));
      return 1;
   }

   struct NetQuit
   {
      ~NetQuit() { NET_Quit(); }
   } net_quit;

   Server server(true, websocket_port, std::move(origins));

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
