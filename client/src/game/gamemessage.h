#pragma once

// GLES3 port of client/src/game/gamemessage.cpp.

#include "framework/frametimer.h"
#include "signal.h"

#include <string>

class GameMessage
{
public:
   GameMessage();
   virtual ~GameMessage() = default;

   void operator=(const GameMessage&);
   GameMessage(const GameMessage& message);

   void setMessage(const std::string&);

   static void setDisplayTime(int time);

   virtual void initialize() {};

   Signal<> expiredSignal;

protected:
   std::string _message;
   FrameTimer _time;
   int _sender_id;
   int _receiver_id;
   std::string _sender_name;
   std::string _receiver_name;

   static int sDisplayTime;
};
