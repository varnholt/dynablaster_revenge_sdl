#include "frametimer.h"
#include "globaltime.h"
#include "timerhandler.h"

FrameTimer::FrameTimer(const FrameTimer& other)
    : _started(other._started), _single_shot(other._single_shot), _start_time(other._start_time), _interval(other._interval)
{
   if (_started)
   {
      TimerHandler::Instance().addTimer(*this);
   }
}

FrameTimer::~FrameTimer()
{
   if (_started)
   {
      TimerHandler::Instance().removeTimer(*this);
   }
}

FrameTimer& FrameTimer::operator=(const FrameTimer& other)
{
   if (&other != this)
   {
      _started = other._started;
      _single_shot = other._single_shot;
      _start_time = other._start_time;
      _interval = other._interval;
      if (_started)
      {
         TimerHandler::Instance().addTimer(*this);
      }
   }
   return *this;
}

FrameTimer FrameTimer::currentTime()
{
   FrameTimer time;
   time._start_time = GlobalTime::Instance().getTime();
   return time;
}

bool FrameTimer::isValid() const
{
   return _started;
}

void FrameTimer::setSingleShot(bool single_shot)
{
   _single_shot = single_shot;
}

FrameTimer FrameTimer::addMSecs(float ms) const
{
   FrameTimer time;
   time._start_time = _start_time + ms * 0.001f;
   return time;
}

float FrameTimer::msecsTo(const FrameTimer& other) const
{
   return (other._start_time - _start_time) * 1000.0f;
}

void FrameTimer::setInterval(float interval)
{
   _interval = interval * 0.001f;
}

float FrameTimer::interval() const
{
   return _interval * 1000.0f;
}

void FrameTimer::start()
{
   _start_time = GlobalTime::Instance().getTime();
   if (!_started)
   {
      _started = true;
      TimerHandler::Instance().addTimer(*this);
   }
}

void FrameTimer::start(float interval)
{
   setInterval(interval);
   start();
}

void FrameTimer::restart()
{
   start();
}

void FrameTimer::stop()
{
   if (_started)
   {
      _started = false;
      TimerHandler::Instance().removeTimer(*this);
   }
}

float FrameTimer::elapsed() const
{
   if (_started)
   {
      const float current_time = GlobalTime::Instance().getTime();
      return (current_time - _start_time) * 1000.0f;
   }
   return 0.0f;
}

bool FrameTimer::update()
{
   if (_started && _interval > 0.0f)
   {
      const float current_time = GlobalTime::Instance().getTime();
      if (current_time >= _start_time + _interval)
      {
         timeoutSignal();
         if (_single_shot)
         {
            _started = false;
            return true;
         }
         _start_time = current_time;
      }
   }
   return false;
}
