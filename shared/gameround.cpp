#include "gameround.h"

GameRound::GameRound()
  : _current(0),
    _count(0)
{
}


void GameRound::reset()
{
   // setCount(0);
   setCurrent(0);
}


void GameRound::next()
{
   setCurrent(getCurrent() + 1);
}


bool GameRound::isFinished() const
{
   return (getCurrent() >= getCount());
}


int32_t GameRound::getCurrent() const
{
   return _current;
}


int32_t GameRound::getCount() const
{
   return _count;
}


void GameRound::setCurrent(int32_t current)
{
   _current = current;
}


void GameRound::setCount(int32_t count)
{
   _count = count;
}
