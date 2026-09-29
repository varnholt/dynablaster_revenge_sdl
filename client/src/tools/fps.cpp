#include "fps.h"

#include <SDL3/SDL.h>

#include <algorithm>

namespace
{
uint32_t GetTickCount()
{
   return static_cast<uint32_t>(SDL_GetTicks());
}
}  // namespace

FPS::FPS(int32_t period) : _period(std::max(period, 1))
{
}

void FPS::next()
{
   _frame++;
   if (_frame >= _period)
   {
      const auto current_time = static_cast<int32_t>(GetTickCount());
      _current_fps = static_cast<float>(_frame) * 1000 / (current_time - _time);
      _time = current_time;
      _frame = 0;
   }
}

float FPS::get()
{
   return _current_fps;
}
