#include "timerhandler.h"

#include <utility>

TimerHandler::~TimerHandler()
{
   auto iterator = _timers.begin();
   while (iterator != _timers.end())
   {
      FrameTimer* timer = *iterator;
      iterator = _timers.erase(iterator);
      if (timer->_delete)
      {
         delete timer;
      }
   }
}

void TimerHandler::addTimer(FrameTimer* timer)
{
   if (timer)
   {
      _timers.insert(timer);
   }
}

void TimerHandler::removeTimer(FrameTimer* timer)
{
   _timers.erase(timer);
}

void TimerHandler::update()
{
   auto iterator = _timers.begin();
   while (iterator != _timers.end())
   {
      FrameTimer* timer = *iterator;

      if (timer && timer->update())
      {
         iterator = _timers.erase(iterator);
         if (timer->_delete)
         {
            delete timer;
         }
      }
      else
      {
         iterator++;
      }
   }
}

void TimerHandler::singleShot(float ms, std::function<void()> callback)
{
   auto* timer = new FrameTimer();
   timer->setSingleShot(true);
   timer->setInterval(ms);
   timer->_delete = true;
   timer->timeoutSignal.connect(std::move(callback));
   timer->start();
}
