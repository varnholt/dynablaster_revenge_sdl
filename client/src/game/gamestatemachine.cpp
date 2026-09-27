#include "gamestatemachine.h"


GameStateMachine* GameStateMachine::sInstance = 0;


GameStateMachine::GameStateMachine()
 : mState(Constants::GameStopped)
{
   sInstance = this;
}


GameStateMachine *GameStateMachine::getInstance()
{
   if (!sInstance)
   {
      sInstance = new GameStateMachine();
   }

   return sInstance;
}


void GameStateMachine::setState(Constants::GameState nextState)
{
   if (mState != nextState)
   {
      mState = nextState;
      stateChangedSignal();
   }
}


Constants::GameState GameStateMachine::getState() const
{
   return mState;
}
