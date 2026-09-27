#pragma once

#include "signal.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_set>

// mimics QTimer's instance API (setInterval/start/stop/isActive/timeout) plus its static
// singleShot(), backed by a process-wide registry ticked once per frame via update() - the
// caller (main.cpp's per-frame loop, or server-sdl's poll loop) decides the tick rate. A timer
// only fires from the thread that started it; update() only processes timers owned by the
// calling thread.
class Timer
{
public:
   Timer() = default;
   ~Timer();

   Timer(const Timer&) = delete;
   Timer& operator=(const Timer&) = delete;

   void setInterval(int32_t milliseconds);
   [[nodiscard]] int32_t interval() const;
   void start();
   void start(int32_t milliseconds);
   void stop();
   [[nodiscard]] bool isActive() const;

   Signal<> timeoutSignal;

   static void singleShot(int32_t milliseconds, std::function<void()> callback);
   static void update();

private:
   std::chrono::milliseconds _interval{0};
   std::chrono::steady_clock::time_point _start_time;
   std::thread::id _owner_thread{};
   bool _active = false;

   static std::mutex _mutex;
   static std::unordered_set<Timer*> _timers;
};
