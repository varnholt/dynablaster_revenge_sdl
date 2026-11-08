#pragma once

#include <cstdint>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

struct ServerOptions
{
   uint16_t websocket_port = 0;
   std::vector<std::string> websocket_origins;
   bool show_help = false;

   static std::expected<ServerOptions, std::string> parse(int argc, char** argv);
   static constexpr std::string_view usage = "Usage: dynablaster_server [--websocket-port PORT --origin URL [--origin URL ...]]";
};
