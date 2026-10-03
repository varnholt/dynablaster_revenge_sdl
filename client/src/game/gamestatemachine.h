#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

// shared
#include "constants.h"
#include "gamesignal.h"

class GameStateMachine
{
public:
   //! constructor, use getInstance()
   GameStateMachine() = default;

   //! instance getter, the instance is never destroyed
   static GameStateMachine& getInstance();

   void setState(Constants::GameState next_state);

   Constants::GameState getState() const;

   Signal<> stateChangedSignal;

protected:
   Constants::GameState _state = Constants::GameStopped;
};

#endif  // GAMESTATEMACHINE_H
