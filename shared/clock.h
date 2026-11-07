#pragma once

#include <chrono>

// process-wide time source for game logic: Timer, ElapsedTimer and the bots read it instead of
// steady_clock. It follows the real clock unless a headless simulation switches it to simulated
// time, which then only moves when advance() is called. Simulated time is meant for a single
// threaded simulation loop and is never enabled by the game itself.
class Clock
{
public:
   using time_point = std::chrono::steady_clock::time_point;
   using duration = std::chrono::steady_clock::duration;

   //! the current time, real or simulated
   [[nodiscard]] static time_point now();

   //! switch to simulated time, which starts at the current real time and stands still until advanced
   static void enableSimulation();

   //! back to the real clock
   static void disableSimulation();

   [[nodiscard]] static bool isSimulated();

   //! move simulated time forward, does nothing on the real clock
   static void advance(duration delta);

private:
   static bool _simulated;
   static time_point _simulated_now;
};
