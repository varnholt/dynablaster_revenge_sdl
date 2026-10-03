#include "gamestatemachine.h"

#include "tools/singleton.h"

GameStateMachine& GameStateMachine::getInstance()
{
   return Singleton<GameStateMachine>::Instance();
}

void GameStateMachine::setState(Constants::GameState next_state)
{
   if (_state != next_state)
   {
      _state = next_state;
      stateChangedSignal();
   }
}

Constants::GameState GameStateMachine::getState() const
{
   return _state;
}
