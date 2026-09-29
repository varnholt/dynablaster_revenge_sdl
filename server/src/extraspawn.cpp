// header
#include "extraspawn.h"

// shared
#include "constants.h"
#include "map.h"

ExtraSpawn::ExtraSpawn()
{
   _spawn_timer.setInterval(SERVER_SPAWN_INTERVAL);
}

bool ExtraSpawn::isEnabled() const
{
   return _enabled;
}

void ExtraSpawn::setEnabled(bool enabled)
{
   _enabled = enabled;
}

void ExtraSpawn::setMap(Map* map)
{
   _map = map;
}

void ExtraSpawn::activateSpawnTimer()
{
   _spawn_timer.start();
}

//! start condition: once this returned true, extras are spawned all the time
bool ExtraSpawn::isExtraAvailable() const
{
   return _map->isHiddenExtraAvailable();
}

bool ExtraSpawn::isSpawnTimerActive() const
{
   return _spawn_timer.isActive();
}

Map* ExtraSpawn::getMap() const
{
   return _map;
}

void ExtraSpawn::reset()
{
   _spawn_timer.stop();
}

void ExtraSpawn::update()
{
   if (isEnabled() && !isSpawnTimerActive() && !isExtraAvailable())
   {
      activateSpawnTimer();
   }
}
