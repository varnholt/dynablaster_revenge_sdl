#pragma once

// game
#include "level.h"

#include <memory>
#include <string>

class LevelFactory
{
public:
   static std::unique_ptr<Level> getLevelInstance(Level::LevelType type);
   static std::unique_ptr<Level> getLevelInstance(const std::string& level_name);
};
