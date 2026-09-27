#ifndef SHAKEPACKETHANDLER_H
#define SHAKEPACKETHANDLER_H

// shared
#include "timer.h"

// forward declarations
class Game;


class ExtraShakePacketHandler
{
public:

   ExtraShakePacketHandler();

   void setEnabled(bool enabled);

   void setGame(Game* game);
   Game* getGame() const;

protected:

   void check();


   Game* mGame;

   Timer mCheckTimer;
};

#endif // SHAKEPACKETHANDLER_H
