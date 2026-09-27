#pragma once

#include <cstdint>

class GameRound
{
   public:

      GameRound();

      void reset();

      void next();

      [[nodiscard]] bool isFinished() const;

      [[nodiscard]] int32_t getCurrent() const;

      [[nodiscard]] int32_t getCount() const;

      void setCurrent(int32_t current);

      void setCount(int32_t count);


   protected:

      int32_t _current;

      int32_t _count;
};
