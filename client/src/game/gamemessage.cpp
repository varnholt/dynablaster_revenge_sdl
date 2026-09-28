// GLES3 port of client/src/game/gamemessage.cpp.

#include "gamemessage.h"
#include "framework/timerhandler.h"

int GameMessage::sDisplayTime = 10000;

GameMessage::GameMessage() : _sender_id(-1), _receiver_id(-1)
{
   _time = FrameTimer::currentTime();

   TimerHandler::singleShot(sDisplayTime, [this]() { expiredSignal(); });
}

void GameMessage::operator=(const GameMessage& message)
{
   _message = message._message;
   _time = message._time;
   _sender_id = message._sender_id;
   _receiver_id = message._receiver_id;
   _receiver_name = message._receiver_name;
   _sender_name = message._sender_name;
}

GameMessage::GameMessage(const GameMessage& message)
{
   _message = message._message;
   _time = message._time;
   _sender_id = message._sender_id;
   _receiver_id = message._receiver_id;
   _receiver_name = message._receiver_name;
   _sender_name = message._sender_name;

   TimerHandler::singleShot(sDisplayTime, [this]() { expiredSignal(); });
}

void GameMessage::setMessage(const std::string& message)
{
   _message = message;
}

void GameMessage::setDisplayTime(int time)
{
   sDisplayTime = time;
}
