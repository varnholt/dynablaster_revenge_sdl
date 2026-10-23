#include "enemytype.h"

#include <array>
#include <cstddef>

namespace
{
constexpr std::array<std::string_view, static_cast<size_t>(EnemyType::Count)> NAMES = {
   "ballom",   "ekutopu",   "boyon", "pass",  "pomori",  "telpio", "onil",     "gacha",    "uotan",
   "boma",     "minvo",     "buffer", "flapper", "sashakin", "nagachamu", "korisuke", "maron", "ojin",
   "pontan",   "arion",     "bubbles", "cloud", "warp_man", "warp_man_child", "spidfire", "black_bomberman", "bomberman"
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
      case EnemyType::Arion:
      case EnemyType::Bubbles:
      case EnemyType::WarpMan:
      case EnemyType::Spidfire:
      case EnemyType::BlackBomberman:
         return true;
      default:
         return false;
   }
}
