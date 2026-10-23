#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

//! story mode enemies; the name is both the behaviour script and the model file
enum class EnemyType : int8_t
{
   Ballom,
   Ekutopu,
   Boyon,
   Pass,
   Pomori,
   Telpio,
   Onil,
   Gacha,
   Uotan,
   Boma,
   Minvo,
   Buffer,
   Flapper,
   Sashakin,
   Nagachamu,
   Korisuke,
   Maron,
   Ojin,
   Pontan,
   Arion,
   Bubbles,
   Cloud,
   WarpMan,
   WarpManChild,
   Spidfire,
   BlackBomberman,
   Bomberman,
   Count
};

[[nodiscard]] std::string_view getEnemyTypeName(EnemyType type);
[[nodiscard]] std::optional<EnemyType> getEnemyType(std::string_view name);
[[nodiscard]] bool isBoss(EnemyType type);
