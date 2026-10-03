#include "levelfactory.h"

// levels
#include "castle/levelcastle.h"
#include "mansion/levelmansion.h"
#include "space/levelspace.h"

// game
#include "constants.h"

#include <array>
#include <filesystem>

std::unique_ptr<Level> LevelFactory::getLevelInstance(Level::LevelType type)
{
   switch (type)
   {
      case Level::LevelCastle:
         return std::make_unique<LevelCastle>();
      case Level::LevelMansion:
         return std::make_unique<LevelMansion>();
      case Level::LevelSpace:
         return std::make_unique<LevelSpace>();
      default:
         return nullptr;
   }
}

std::unique_ptr<Level> LevelFactory::getLevelInstance(const std::string& level_name)
{
   Level::LevelType level_type = Level::LevelCastle;

   for (const auto type : std::array{Level::LevelCastle, Level::LevelMansion, Level::LevelSpace})
   {
      const auto directory = Level::getLevelDirectoryName(type);

      // a build can leave level data out (the web build only ships the castle)
      if (level_name.ends_with(directory) && std::filesystem::exists("data/" + directory + "/level.hjb"))
      {
         level_type = type;
         break;
      }
   }

   return getLevelInstance(level_type);
}
