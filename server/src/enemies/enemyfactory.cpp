#include "enemyfactory.h"

#include "scriptedenemy.h"

std::unique_ptr<Enemy> EnemyFactory::create(int16_t id, EnemyType type, EnemyWorld& world)
{
   auto enemy = std::make_unique<ScriptedEnemy>(id, type, world);
   enemy->initialize();
   return enemy;
}
