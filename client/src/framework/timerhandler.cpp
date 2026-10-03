#include "timerhandler.h"

#include <algorithm>
#include <iterator>
#include <utility>

TimerHandler::~TimerHandler()
{
   _timers.clear();
   _single_shots.clear();
}

void TimerHandler::addTimer(FrameTimer& timer)
{
   if (std::ranges::none_of(_timers, [&timer](const FrameTimer& candidate) { return &candidate == &timer; }))
   {
      _timers.emplace_back(timer);
   }
}

void TimerHandler::removeTimer(const FrameTimer& timer)
{
   const auto iterator = std::ranges::find_if(_timers, [&timer](const FrameTimer& candidate) { return &candidate == &timer; });
   if (iterator == _timers.end())
   {
      return;
   }

   if (std::distance(_timers.begin(), iterator) <= _cursor)
   {
      _cursor--;
   }
   _timers.erase(iterator);
}

void TimerHandler::update()
{
   for (_cursor = 0; _cursor < std::ssize(_timers); _cursor++)
   {
      FrameTimer& timer = _timers[_cursor];

      if (timer.update())
      {
         const bool owned = timer._delete;
         removeTimer(timer);
         if (owned)
         {
            std::erase_if(_single_shots, [&timer](const std::unique_ptr<FrameTimer>& candidate) { return candidate.get() == &timer; });
         }
      }
   }
   _cursor = -1;
}

void TimerHandler::singleShot(float ms, std::function<void()> callback)
{
   TimerHandler& handler = Instance();
   FrameTimer& timer = *handler._single_shots.emplace_back(std::make_unique<FrameTimer>());
   timer.setSingleShot(true);
   timer.setInterval(ms);
   timer._delete = true;
   timer.timeoutSignal.connect(std::move(callback));
   timer.start();
}
