#ifndef EXTRASPAWN_H
#define EXTRASPAWN_H

// shared
#include "gamesignal.h"
#include "timer.h"

// forward declarations
class Map;

class ExtraSpawn
{
public:
   ExtraSpawn();

   //! check if spawning is enabled
   bool isEnabled() const;

   //! time to spawn an extra
   Signal<> spawnSignal;

   //! reset spawn state
   void reset();

   //! check if spawn needs to be activated
   void update();

   //! setter for enabled flag (completely enable or disable extra spawning)
   void setEnabled(bool enabled);

   //! setter for map
   void setMap(Map* map);

protected:
   //! activate spawn timer
   void activateSpawnTimer();

   //! check if extra is available
   bool isExtraAvailable() const;

   //! check if spawn timer is already active
   bool isSpawnTimerActive() const;

   //! getter for map
   Map* getMap() const;

   //! spawning is enabled
   bool _enabled = false;

   //! spawn timer
   Timer _spawn_timer;

   //! game's map
   Map* _map = nullptr;
};

#endif  // EXTRASPAWN_H
