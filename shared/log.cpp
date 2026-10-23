#include "log.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <iostream>
#include <mutex>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace
{
constexpr std::string_view color_reset = "\033[0m";
constexpr std::string_view color_yellow = "\033[33m";
constexpr std::string_view color_red = "\033[31m";
constexpr std::string_view color_bright_red = "\033[91m";

// the client's embedded server logs from its own thread
std::mutex _mutex;

void enableWindowsAnsiColors()
{
#ifdef _WIN32
   static const bool initialized = []()
   {
      const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
      DWORD mode = 0;
      if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode))
      {
         SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
      }
      return true;
   }();
   static_cast<void>(initialized);
#endif
}

constexpr std::string_view colorForLevel(Log::Level level)
{
   switch (level)
   {
      case Log::Level::Warning:
         return color_yellow;
      case Log::Level::Error:
         return color_red;
      case Log::Level::Fatal:
         return color_bright_red;
      default:
         return "";
   }
}

// std::localtime hands out a shared static tm, the embedded server logs from its own thread
std::string localTime()
{
   const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

   std::tm local_time{};
#ifdef _WIN32
   if (localtime_s(&local_time, &time) != 0)
#else
   if (localtime_r(&time, &local_time) == nullptr)
#endif
   {
      return {};
   }

   return std::format(
      "{:04}-{:02}-{:02} {:02}:{:02}:{:02}",
      local_time.tm_year + 1900,
      local_time.tm_mon + 1,
      local_time.tm_mday,
      local_time.tm_hour,
      local_time.tm_min,
      local_time.tm_sec
   );
}

void log(Log::Level level, std::string_view message, const std::source_location& source)
{
   const auto color = colorForLevel(level);
   const auto reset = color.empty() ? std::string_view{} : color_reset;

   std::lock_guard<std::mutex> lock(_mutex);
   enableWindowsAnsiColors();
   std::cout << std::format(
                   "{}[{}] {} | {}: {}{}", color, static_cast<char>(level), localTime(), Log::parseSourceTag(source), message, reset
                )
             << std::endl;
}
}  // namespace

void Log::info(std::string_view message, const std::source_location& source)
{
   log(Level::Info, message, source);
}

void Log::warning(std::string_view message, const std::source_location& source)
{
   log(Level::Warning, message, source);
}

void Log::error(std::string_view message, const std::source_location& source)
{
   log(Level::Error, message, source);
}

void Log::fatal(std::string_view message, const std::source_location& source)
{
   log(Level::Fatal, message, source);
   std::abort();
}

std::string Log::parseSourceTag(const std::source_location& source)
{
   std::string function_name = source.function_name();
   function_name = function_name.substr(0, function_name.find('('));

   // msvc puts the calling convention and return type in front
   constexpr std::string_view cdecl_prefix = "__cdecl ";
   if (const auto pos = function_name.find(cdecl_prefix); pos != std::string::npos)
   {
      function_name.erase(0, pos + cdecl_prefix.size());
   }

   return std::format("{}:{}:{}", std::filesystem::path{source.file_name()}.filename().string(), function_name, source.line());
}
