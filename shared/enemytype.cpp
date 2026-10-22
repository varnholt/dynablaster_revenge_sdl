#include "enemytype.h"

#include <array>
#include <cstddef>

namespace
{
constexpr std::array<std::string_view, static_cast<size_t>(EnemyType::Count)> NAMES = {
   "balloon", "octopus", "droplet", "tiger",  "imp",   "totem",   "onion", "croc",    "blob",
   "frog",    "grinner", "gourd",   "hothead", "jester", "statue", "spinner", "sprout", "chomper",
   "coin",    "serpent", "puffer",  "cloud",  "cyclops", "eyeball", "crab", "shadow_bomber", "henchman"
};
}

std::string_view getEnemyTypeName(EnemyType type)
{
   const auto index = static_cast<size_t>(type);
   return index < NAMES.size() ? NAMES[index] : std::string_view{};
}

std::optional<EnemyType> getEnemyType(std::string_view name)
{
   for (size_t i = 0; i < NAMES.size(); i++)
   {
      if (NAMES[i] == name)
      {
         return static_cast<EnemyType>(i);
      }
   }

   return std::nullopt;
}

bool isBoss(EnemyType type)
{
   switch (type)
   {
      case EnemyType::Serpent:
      case EnemyType::Puffer:
      case EnemyType::Cyclops:
      case EnemyType::Crab:
      case EnemyType::ShadowBomber:
         return true;
      default:
         return false;
   }
}
