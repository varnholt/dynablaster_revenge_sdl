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
   std::string mMessage;
   FrameTimer mTime;
   int mSenderId;
   int mReceiverId;
   std::string mSenderName;
   std::string mReceiverName;

   static int sDisplayTime;
};
