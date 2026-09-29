#pragma once

#include <cstdint>
#include <string>

#include "constants.h"

class CreateGameData
{
public:
   CreateGameData() = default;
   virtual ~CreateGameData() = default;

   std::string _name;
   std::string _level;
   int32_t _rounds = 0;

   // game duration (s)
   int32_t _duration = 0;

   int32_t _max_players = 0;
   bool _extra_bomb_enabled = false;
   bool _extra_flame_enabled = false;
   bool _extra_speedup_enabled = false;
   bool _extra_kick_enabled = false;
   bool _extra_skulls_enabled = false;
   Constants::Dimension _dimension = Constants::Dimension13x11;
};
