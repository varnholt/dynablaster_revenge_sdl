#include "serveroptions.h"

#include "constants.h"

#include <charconv>

std::expected<ServerOptions, std::string> ServerOptions::parse(int argc, char** argv)
{
   ServerOptions options;
   for (int i = 1; i < argc; ++i)
   {
      const std::string_view option(argv[i]);
      if (option == "--help")
      {
         options.show_help = true;
         return options;
      }
      if ((option != "--websocket-port" && option != "--origin") || i + 1 == argc)
      {
         return std::unexpected("Expected --websocket-port PORT or --origin URL; use --help for usage");
      }
      const std::string_view value(argv[++i]);
      if (option == "--origin")
      {
         if ((!value.starts_with("http://") && !value.starts_with("https://")) || value.find_first_of("\r\n") != std::string_view::npos)
         {
            return std::unexpected("An origin must be an http:// or https:// page origin");
         }
         options.websocket_origins.emplace_back(value);
      }
      else
      {
         unsigned port = 0;
         const auto result = std::from_chars(value.data(), value.data() + value.size(), port);
         if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || port == 0 || port > 65535 || port == SERVER_PORT)
         {
            return std::unexpected("WebSocket port must be 1..65535 and differ from the game TCP port");
         }
         options.websocket_port = static_cast<uint16_t>(port);
      }
   }
   if (options.websocket_port && options.websocket_origins.empty())
   {
      return std::unexpected("--websocket-port requires at least one --origin URL");
   }
   if (!options.websocket_port && !options.websocket_origins.empty())
   {
      return std::unexpected("--origin requires --websocket-port PORT");
   }
   return options;
}
