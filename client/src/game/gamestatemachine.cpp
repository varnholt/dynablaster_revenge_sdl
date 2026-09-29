#include "gamestatemachine.h"

GameStateMachine* GameStateMachine::s_instance = nullptr;

GameStateMachine::GameStateMachine()
{
   s_instance = this;
}

GameStateMachine* GameStateMachine::getInstance()
{
   if (!s_instance)
   {
      s_instance = new GameStateMachine();
   }

   return s_instance;
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
