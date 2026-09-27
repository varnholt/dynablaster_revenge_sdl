#ifndef LEVELFACTORY_H
#define LEVELFACTORY_H

// game
#include "level.h"

#include <string>

class LevelFactory
{
   public:

      LevelFactory();

      static LevelFactory* getFactoryInstance();

      static Level* getLevelInstance(Level::LevelType type);
      static Level* getLevelInstance(const std::string& typeName);

   private:

      static LevelFactory* sInstance;
};

#endif // LEVELFACTORY_H
