#ifndef SHAKEPACKETHANDLER_H
#define SHAKEPACKETHANDLER_H

// shared
#include "timer.h"

// forward declarations
class Game;

class ExtraShakePacketHandler
{
public:
   explicit ExtraShakePacketHandler(Game& game);

   void setEnabled(bool enabled);

   Game& getGame() const;

protected:
   void check();

   Game& _game;

   Timer _check_timer;
};

#endif  // SHAKEPACKETHANDLER_H
