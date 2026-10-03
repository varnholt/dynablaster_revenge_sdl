#pragma once

// printf-style debug/warning/fatal logging, matching the call-site shape every real qDebug()/
// qWarning()/qFatal() usage in this tree already uses (plain format string + printf arguments,
// never the Qt streaming qDebug() << x style) - so no call site needs to change, just the include.

#include <cstdio>
#include <cstdlib>
#include <string>

namespace logging_detail
{
template <typename... Args>
void print(std::FILE& stream, const std::string& format, const Args&... args)
{
   if constexpr (sizeof...(Args) == 0)
   {
      std::fputs(format.c_str(), &stream);
   }
   else
   {
      std::fprintf(&stream, format.c_str(), args...);
   }

   std::fputc('\n', &stream);
}
}  // namespace logging_detail

template <typename... Args>
void qDebug(const std::string& format, const Args&... args)
{
   logging_detail::print(*stdout, format, args...);
}

template <typename... Args>
void qWarning(const std::string& format, const Args&... args)
{
   logging_detail::print(*stderr, format, args...);
}

template <typename... Args>
[[noreturn]] void qFatal(const std::string& format, const Args&... args)
{
   logging_detail::print(*stderr, format, args...);
   std::abort();
}
