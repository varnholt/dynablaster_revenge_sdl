#include "menucontrollercursoranimation.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numbers>

namespace
{
constexpr float duration_ms = 400.0f;
constexpr float max_manhattan_length = 200.0f;
}  // namespace

void MenuControllerCursorAnimation::add(int32_t x, int32_t y)
{
   _destinations.push_back({x, y});
}

bool MenuControllerCursorAnimation::isBusy() const
{
   return !_destinations.empty();
}

void MenuControllerCursorAnimation::update(uint64_t now_ms)
{
   if (_destinations.empty())
   {
      return;
   }

   if (!_running)
   {
      _origin = _position;
      _start_ms = now_ms;
      _running = true;
   }

   // short distances are covered faster
   const Point& destination = _destinations.front();
   const auto distance = static_cast<float>(std::abs(_origin.x - destination.x) + std::abs(_origin.y - destination.y));
   const float duration = duration_ms * std::min(1.0f, distance / max_manhattan_length);
   const auto elapsed = static_cast<float>(now_ms - _start_ms);

   const float normed = duration > 0.0f ? std::min(1.0f, elapsed / duration) : 1.0f;
   const float a = 0.5f * (1.0f + std::cos(std::numbers::pi_v<float> * normed));
   const float b = 1.0f - a;
   _position.x = static_cast<int32_t>(std::lround(a * _origin.x + b * destination.x));
   _position.y = static_cast<int32_t>(std::lround(a * _origin.y + b * destination.y));
   movedSignal(_position.x, _position.y);

   if (elapsed >= duration)
   {
      _destinations.pop_front();
      _running = false;
      doneSignal();
   }
}

void MenuControllerCursorAnimation::setPosition(int32_t x, int32_t y)
{
   _position = {x, y};
}

int32_t MenuControllerCursorAnimation::getX() const
{
   return _position.x;
}

int32_t MenuControllerCursorAnimation::getY() const
{
   return _position.y;
}
