#pragma once

#include <source_location>
#include <string>
#include <string_view>

// timestamped log lines tagged with their call site, after deceptus' Log; messages are built with
// std::format at the call site
namespace Log
{

enum class Level : char
{
   Info = 'i',
   Warning = 'w',
   Error = 'e',
   Fatal = 'f',
};

//! something worth knowing happened
void info(std::string_view message, const std::source_location& source = std::source_location::current());

//! something went wrong but the program carries on
void warning(std::string_view message, const std::source_location& source = std::source_location::current());

//! something failed
void error(std::string_view message, const std::source_location& source = std::source_location::current());

//! logs and aborts, the program can't go on
[[noreturn]] void fatal(std::string_view message, const std::source_location& source = std::source_location::current());

//! compact "file:function:line" tag of a call site
[[nodiscard]] std::string parseSourceTag(const std::source_location& source);

}  // namespace Log
