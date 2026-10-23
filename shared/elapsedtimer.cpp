#include "elapsedtimer.h"

void ElapsedTimer::start()
{
   _start_time = std::chrono::steady_clock::now();
   _paused_at.reset();
}

int64_t ElapsedTimer::restart()
{
   const auto elapsed_ms = elapsed();
   start();
   return elapsed_ms;
}

int64_t ElapsedTimer::elapsed() const
{
   const auto now = _paused_at.value_or(std::chrono::steady_clock::now());
   return std::chrono::duration_cast<std::chrono::milliseconds>(now - _start_time).count();
}

void ElapsedTimer::setPaused(bool paused)
{
   if (paused == _paused_at.has_value())
   {
      return;
   }

   const auto now = std::chrono::steady_clock::now();

   if (paused)
   {
      _paused_at = now;
   }
   else
   {
      _start_time += now - *_paused_at;
      _paused_at.reset();
   }
}
