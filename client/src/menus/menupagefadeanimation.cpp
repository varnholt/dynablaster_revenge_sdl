#include "menupagefadeanimation.h"

#include <cmath>
#include <numbers>

void MenuPageFadeAnimation::initialize()
{
}

float MenuPageFadeAnimation::getAlpha() const
{
   return _alpha;
}

void MenuPageFadeAnimation::setAlpha(float alpha)
{
   _alpha = alpha;
}

void MenuPageFadeAnimation::setFadeIn(bool fade_in)
{
   _fade_in = fade_in;
}

void MenuPageFadeAnimation::start()
{
   _alpha = _fade_in ? 0.0f : 1.0f;
   _stopped = false;
   _elapsed.restart();
}

void MenuPageFadeAnimation::animate()
{
   if (_stopped)
   {
      return;
   }

   constexpr float half_pi = std::numbers::pi_v<float> * 0.5f;
   const float elapsed = _elapsed.elapsed() * 0.002f;

   float value = 0.0f;

   if (_fade_in)
   {
      value = (elapsed <= half_pi) ? std::sin(elapsed) : 1.0f;
   }
   else
   {
      value = (elapsed <= half_pi) ? std::cos(elapsed) : 0.0f;
   }

   setAlpha(value);

   if (elapsed > half_pi)
   {
      _stopped = true;
      stoppedSignal();
   }
}

bool MenuPageFadeAnimation::isStopped() const
{
   return _stopped;
}

void MenuPageFadeAnimation::setStopped(bool value)
{
   _stopped = value;
}
