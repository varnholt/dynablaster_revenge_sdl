#pragma once

#include "gamesignal.h"

class FrameTimer
{
public:
   FrameTimer() = default;
   FrameTimer(const FrameTimer& other);
   ~FrameTimer();

   FrameTimer& operator=(const FrameTimer& other);

   static FrameTimer currentTime();

   bool isValid() const;

   FrameTimer addMSecs(float ms) const;
   float msecsTo(const FrameTimer& other) const;

   void start();
   void start(float interval);
   void restart();
   void stop();

   float elapsed() const;

   bool update();

   float interval() const;
   void setInterval(float ms);

   void setSingleShot(bool single_shot);

   Signal<> timeoutSignal;

private:
   friend class TimerHandler;

   bool _started = false;
   bool _single_shot = false;
   float _start_time = 0.0f;  // seconds
   float _interval = 0.0f;    // seconds
   bool _delete = false;      // owned by TimerHandler, destroyed once it fired
};
