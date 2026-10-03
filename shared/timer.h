#pragma once

#include "gamesignal.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <thread>

// interval timer plus static singleShot(), backed by a process-wide registry ticked via update() -
// the caller's loop decides the tick rate. A timer only fires from the thread that started it;
// update() only processes timers owned by the calling thread.
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
   [[nodiscard]] static uint64_t nextId();

   // key into the registry, timers are neither copyable nor movable
   uint64_t _id = nextId();
   std::chrono::milliseconds _interval{0};
   std::chrono::steady_clock::time_point _start_time;
   std::thread::id _owner_thread{};
   bool _active = false;

   static std::mutex _mutex;

   // started timers by id, fired in id (creation) order
   static std::map<uint64_t, std::reference_wrapper<Timer>> _timers;
};
