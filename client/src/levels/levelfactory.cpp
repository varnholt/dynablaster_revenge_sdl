// header
#include "levelfactory.h"

// levels
#include "castle/levelcastle.h"
#include "mansion/levelmansion.h"

// game
#include "constants.h"

#include <array>
#include <filesystem>

LevelFactory* LevelFactory::sInstance = 0;


LevelFactory::LevelFactory()
{
   sInstance = this;
}


LevelFactory *LevelFactory::getFactoryInstance()
{
   return sInstance ? sInstance : new LevelFactory();
}


Level *LevelFactory::getLevelInstance(Level::LevelType levelType)
{
   Level* level = 0;

   switch (levelType)
   {
      case Level::LevelCastle:
         level = new LevelCastle();
         break;

      case Level::LevelMansion:
         level = new LevelMansion();
         break;

      default:
         break;
   }

   return level;
}


Level *LevelFactory::getLevelInstance(const std::string &levelName)
{
   Level::LevelType level_type = Level::LevelCastle;

   for (const auto type : std::array{Level::LevelCastle, Level::LevelMansion})
   {
      const auto directory = Level::getLevelDirectoryName(type);

      // a build can leave level data out (the web build only ships the castle)
      if (levelName.ends_with(directory) && std::filesystem::exists("data/" + directory + "/level.hjb"))
      {
         level_type = type;
         break;
      }
   }

   return getLevelInstance(level_type);
}
