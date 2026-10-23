#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "constants.h"
#include "enemytype.h"

//! single screen or one of the stages scrolling over two screens
enum class StoryScroll : int8_t
{
   None,
   Horizontal,
   Vertical
};

//! one of the original game's 64 stages
struct StoryStage
{
   std::string_view _name;
   StoryScroll _scroll = StoryScroll::None;

   //! the extra hidden under one soft block, 0 for none (boss stages)
   int32_t _item = 0;

   std::string_view _password;
   std::vector<EnemyType> _enemies;

   [[nodiscard]] int32_t getWidth() const
   {
      return _scroll == StoryScroll::Horizontal ? 29 : 13;
   }

   [[nodiscard]] int32_t getHeight() const
   {
      return _scroll == StoryScroll::Vertical ? 23 : 11;
   }

   [[nodiscard]] int32_t getWorld() const
   {
      return _name.empty() ? 1 : _name[0] - '0';
   }

   [[nodiscard]] bool isBossStage() const
   {
      return _name.size() > 2 && _name[2] == '8';
   }
};

[[nodiscard]] const std::vector<StoryStage>& getStoryStages();
