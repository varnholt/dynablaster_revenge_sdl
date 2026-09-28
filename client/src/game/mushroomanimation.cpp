#include "mushroomanimation.h"

#include "framework/globaltime.h"

#include <algorithm>

namespace
{
constexpr float TIME_FADE = 1.0f;
}

void MushroomAnimation::start()
{
   if (!_active)
   {
      _intensity = 0.0f;
      _active = true;
   }

   const float time = GlobalTime::Instance()->getTime();
   _time = time;
   _start_time = time;
   _aborted = false;
}

void MushroomAnimation::abort()
{
   _aborted = true;
}

void MushroomAnimation::update()
{
   const float time = GlobalTime::Instance()->getTime();
   const float dt = time - _time;
   _time = time;

   const float elapsed = _time - _start_time;

   if (_aborted)
   {
      _intensity -= dt;
   }
   else if (elapsed < TIME_FADE)
   {
      _intensity += dt;
   }
   else if (elapsed > TIME_FADE)
   {
      _intensity = 1.0f;
   }

   if (_intensity <= 0.0f)
   {
      _active = false;
   }

   _intensity = std::min(_intensity, 1.0f);
}

bool MushroomAnimation::isActive() const
{
   return _active;
}

float MushroomAnimation::getIntensity() const
{
   return _intensity;
}
