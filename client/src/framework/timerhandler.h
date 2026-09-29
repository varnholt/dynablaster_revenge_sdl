#pragma once

#include "frametimer.h"
#include "tools/singleton.h"

#include <functional>
#include <unordered_set>

class TimerHandler : public Singleton<TimerHandler>
{
public:
   TimerHandler() = default;
   ~TimerHandler() override;

   void addTimer(FrameTimer* timer);
   void removeTimer(FrameTimer* timer);

   void update();

   static void singleShot(float ms, std::function<void()> callback);

private:
   // non-owning, except for timers flagged _delete (singleShot()), which are deleted once they fire
   std::unordered_set<FrameTimer*> _timers;
};
