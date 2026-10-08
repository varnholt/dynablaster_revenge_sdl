#pragma once

#include <cstdint>
#include <memory>

// shared
#include "enemytype.h"

class Enemy;
class EnemyWorld;

namespace EnemyFactory
{
//! the enemy of the given type, driven by its script when there is one
[[nodiscard]] std::unique_ptr<Enemy> create(int16_t id, EnemyType type, EnemyWorld& world);
}
