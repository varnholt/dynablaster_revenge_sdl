#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

// mimics QElapsedTimer's start()/restart()/elapsed() shape
class ElapsedTimer
{
public:
   ElapsedTimer() = default;

   void start();
   int64_t restart();
   int64_t elapsed() const;

   //! elapsed() stands still while paused
   void setPaused(bool paused);

private:
   std::chrono::steady_clock::time_point _start_time = std::chrono::steady_clock::now();
   std::optional<std::chrono::steady_clock::time_point> _paused_at;
};
