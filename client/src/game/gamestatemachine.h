#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

// shared
#include "constants.h"
#include "signal.h"


class GameStateMachine
{
   public:

      static GameStateMachine* getInstance();

      void setState(Constants::GameState next_state);

      Constants::GameState getState() const;

      Signal<> stateChangedSignal;


   protected:

      GameStateMachine();

      Constants::GameState _state;

      static GameStateMachine* s_instance;

};

#endif // GAMESTATEMACHINE_H
