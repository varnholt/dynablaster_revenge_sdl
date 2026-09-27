#include "timer.h"

#include <algorithm>
#include <vector>

namespace
{
struct PendingSingleShot
{
   std::chrono::steady_clock::time_point due;
   std::function<void()> callback;
   std::thread::id owner_thread;
};

std::mutex _single_shot_mutex;
std::vector<PendingSingleShot> _pending_single_shots;
}  // namespace

std::mutex Timer::_mutex;
std::unordered_set<Timer*> Timer::_timers;

Timer::~Timer()
{
   std::lock_guard<std::mutex> lock(_mutex);
   _timers.erase(this);
}

void Timer::setInterval(int32_t milliseconds)
{
   _interval = std::chrono::milliseconds(milliseconds);
}

int32_t Timer::interval() const
{
   return static_cast<int32_t>(_interval.count());
}

void Timer::start()
{
   _start_time = std::chrono::steady_clock::now();
   _owner_thread = std::this_thread::get_id();
   _active = true;

   std::lock_guard<std::mutex> lock(_mutex);
   _timers.insert(this);
}

void Timer::start(int32_t milliseconds)
{
   setInterval(milliseconds);
   start();
}

void Timer::stop()
{
   _active = false;

   std::lock_guard<std::mutex> lock(_mutex);
   _timers.erase(this);
}

bool Timer::isActive() const
{
   return _active;
}

void Timer::singleShot(int32_t milliseconds, std::function<void()> callback)
{
   std::lock_guard<std::mutex> lock(_single_shot_mutex);
   _pending_single_shots.push_back(
      {std::chrono::steady_clock::now() + std::chrono::milliseconds(milliseconds), std::move(callback), std::this_thread::get_id()}
   );
}

void Timer::update()
{
   const auto now = std::chrono::steady_clock::now();
   const auto callingThread = std::this_thread::get_id();

   std::vector<Timer*> due;
   {
      std::lock_guard<std::mutex> lock(_mutex);
      for (auto* timer : _timers)
      {
         if (timer->_owner_thread == callingThread && now - timer->_start_time >= timer->_interval)
         {
            due.push_back(timer);
         }
      }
   }

   for (auto* timer : due)
   {
      // advance by whole elapsed intervals rather than snapping to "now", so the long-run rate
      // stays correct even when update() is polled irregularly.
      if (timer->_interval.count() > 0)
      {
         const auto elapsed = now - timer->_start_time;
         const auto intervals = elapsed / timer->_interval;
         timer->_start_time += timer->_interval * intervals;
      }
      else
      {
         timer->_start_time = now;
      }

      timer->timeoutSignal();
   }

   std::vector<std::function<void()>> callbacks;
   {
      std::lock_guard<std::mutex> lock(_single_shot_mutex);
      auto it = std::remove_if(
         _pending_single_shots.begin(),
         _pending_single_shots.end(),
         [now, callingThread, &callbacks](auto& pending)
         {
            if (pending.owner_thread != callingThread || now < pending.due)
            {
               return false;
            }
            callbacks.push_back(std::move(pending.callback));
            return true;
         }
      );
      _pending_single_shots.erase(it, _pending_single_shots.end());
   }

   for (auto& callback : callbacks)
   {
      callback();
   }
}
