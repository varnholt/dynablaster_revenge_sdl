#pragma once

#include "gamesignal.h"

#include <cstdint>
#include <deque>

/// \brief glides the menu cursor (page coordinates) to queued destinations, one after another
class MenuJoystickMouseAnimation
{
public:
   void add(int32_t x, int32_t y);
   bool isBusy() const;

   /// \brief advances the running animation, starts the next queued one
   void update(uint64_t now_ms);

   /// \brief the real mouse moved the cursor
   void setPosition(int32_t x, int32_t y);
   int32_t getX() const;
   int32_t getY() const;

   Signal<int32_t, int32_t> movedSignal;
   Signal<> doneSignal;

private:
   struct Point
   {
      int32_t x = 0;
      int32_t y = 0;
   };

   std::deque<Point> _destinations;
   Point _origin;
   Point _position{960, 540};
   uint64_t _start_ms = 0;
   bool _running = false;
};
