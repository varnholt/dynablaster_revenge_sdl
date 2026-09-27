#pragma once

// printf-style debug/warning/fatal logging, matching the call-site shape every real qDebug()/
// qWarning()/qFatal() usage in this tree already uses (plain format string + varargs, never the
// Qt streaming qDebug() << x style) - so no call site needs to change, just the include.

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

inline void qDebug(const char* format, ...)
{
   va_list args;
   va_start(args, format);
   std::vfprintf(stdout, format, args);
   va_end(args);
   std::fputc('\n', stdout);
}

inline void qWarning(const char* format, ...)
{
   va_list args;
   va_start(args, format);
   std::vfprintf(stderr, format, args);
   va_end(args);
   std::fputc('\n', stderr);
}

[[noreturn]] inline void qFatal(const char* format, ...)
{
   va_list args;
   va_start(args, format);
   std::vfprintf(stderr, format, args);
   va_end(args);
   std::fputc('\n', stderr);
   std::abort();
}
