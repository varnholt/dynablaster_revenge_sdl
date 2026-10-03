#pragma once

#include "frametimer.h"
#include "tools/singleton.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

class TimerHandler : public Singleton<TimerHandler>
{
public:
   TimerHandler() = default;
   ~TimerHandler() override;

   // running timers register themselves, the handler does not own them
   void addTimer(FrameTimer& timer);
   void removeTimer(const FrameTimer& timer);

   void update();

   static void singleShot(float ms, std::function<void()> callback);

private:
   std::vector<std::reference_wrapper<FrameTimer>> _timers;
   std::vector<std::unique_ptr<FrameTimer>> _single_shots;  // owned, destroyed once they fired

   // index of the timer update() is processing, kept valid while callbacks add or remove timers
   std::ptrdiff_t _cursor = -1;
};
