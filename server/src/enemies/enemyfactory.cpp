#include "enemyfactory.h"

#include "scriptedenemy.h"

#include <format>

std::unique_ptr<Enemy> EnemyFactory::create(int16_t id, EnemyType type, EnemyWorld& world)
{
   auto enemy = std::make_unique<ScriptedEnemy>(id, type, world, std::format("data/scripts/enemies/{}.lua", getEnemyTypeName(type)));
   enemy->initialize();
   return enemy;
}
