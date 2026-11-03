#include "clock.h"

bool Clock::_simulated = false;
Clock::time_point Clock::_simulated_now;

Clock::time_point Clock::now()
{
   return _simulated ? _simulated_now : std::chrono::steady_clock::now();
}

void Clock::enableSimulation()
{
   _simulated_now = std::chrono::steady_clock::now();
   _simulated = true;
}

void Clock::disableSimulation()
{
   _simulated = false;
}

bool Clock::isSimulated()
{
   return _simulated;
}

void Clock::advance(duration delta)
{
   if (_simulated)
   {
      _simulated_now += delta;
   }
}
