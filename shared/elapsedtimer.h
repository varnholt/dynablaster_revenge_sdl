#pragma once

#include <chrono>
#include <cstdint>

// mimics QElapsedTimer's start()/restart()/elapsed() shape
class ElapsedTimer
{
public:
   ElapsedTimer() = default;

   void start();
   int64_t restart();
   int64_t elapsed() const;

private:
   std::chrono::steady_clock::time_point _start_time = std::chrono::steady_clock::now();
};
