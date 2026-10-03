#include "gameplayback.h"

#include "tools/singleton.h"

GamePlayback& GamePlayback::getInstance()
{
   return Singleton<GamePlayback>::Instance();
}

bool GamePlayback::isReplaying() const
{
   return _replaying;
}

void GamePlayback::setReplaying(bool value)
{
   _replaying = value;
}

void GamePlayback::setRecording(bool recording)
{
   _recording = recording;
}

bool GamePlayback::isRecording() const
{
   return _recording;
}

void GamePlayback::record(const Packet&)
{
}
void GamePlayback::playDemo()
{
}
void GamePlayback::abort()
{
}
void GamePlayback::setPlayerId(int)
{
}
