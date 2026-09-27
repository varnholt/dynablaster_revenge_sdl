#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

// shared
#include "constants.h"
#include "signal.h"


class GameStateMachine
{
   public:

      static GameStateMachine* getInstance();

      void setState(Constants::GameState nextState);

      Constants::GameState getState() const;

      Signal<> stateChangedSignal;


   protected:

      GameStateMachine();

      Constants::GameState mState;

      static GameStateMachine* sInstance;

};

#endif // GAMESTATEMACHINE_H
