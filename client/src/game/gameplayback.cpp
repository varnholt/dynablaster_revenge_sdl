#include "gameplayback.h"

GamePlayback* GamePlayback::s_instance = nullptr;

GamePlayback::GamePlayback()
{
   s_instance = this;
}

GamePlayback* GamePlayback::getInstance()
{
   if (!s_instance)
      new GamePlayback();

   return s_instance;
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

void GamePlayback::record(Packet*) {}
void GamePlayback::playDemo() {}
void GamePlayback::abort() {}
void GamePlayback::setPlayerId(int) {}
