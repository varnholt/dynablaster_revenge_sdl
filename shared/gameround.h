#pragma once

#include <cstdint>

class GameRound
{
public:
   GameRound() = default;

   void reset();
   void next();

   [[nodiscard]] bool isFinished() const;

   [[nodiscard]] int32_t getCurrent() const;
   [[nodiscard]] int32_t getCount() const;

   void setCurrent(int32_t current);
   void setCount(int32_t count);

protected:
   int32_t _current = 0;
   int32_t _count = 0;
};
