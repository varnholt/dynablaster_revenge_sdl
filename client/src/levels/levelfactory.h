#pragma once

// game
#include "level.h"

#include <memory>
#include <string>

class LevelFactory
{
public:
   LevelFactory();

   static LevelFactory* getFactoryInstance();

   static std::unique_ptr<Level> getLevelInstance(Level::LevelType type);
   static std::unique_ptr<Level> getLevelInstance(const std::string& level_name);

private:
   static LevelFactory* _instance;
};
