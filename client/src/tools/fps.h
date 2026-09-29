// frame counter class
// register any rendered frame with next(),
// get the current framerate with get().
// specify an averaging period (in number of frames) in the constructor

#pragma once

#include <cstdint>

class FPS
{
public:
   FPS(int32_t period = 100);
   void next();
   float get();

private:
   int32_t _period = 1;
   float _current_fps = 0.0f;
   int32_t _frame = 0;
   int32_t _time = 0;
};
