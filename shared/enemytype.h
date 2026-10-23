#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

//! story mode enemies; the name is both the behaviour script and the model file
enum class EnemyType : int8_t
{
   Balloon,
   Octopus,
   Droplet,
   Tiger,
   Imp,
   Totem,
   Onion,
   Croc,
   Blob,
   Frog,
   Grinner,
   Gourd,
   Hothead,
   Jester,
   Statue,
   Spinner,
   Sprout,
   Chomper,
   Coin,
   Serpent,
   Puffer,
   Cloud,
   Cyclops,
   Eyeball,
   Crab,
   ShadowBomber,
   Henchman,
   Count
};

[[nodiscard]] std::string_view getEnemyTypeName(EnemyType type);
[[nodiscard]] std::optional<EnemyType> getEnemyType(std::string_view name);
[[nodiscard]] bool isBoss(EnemyType type);
