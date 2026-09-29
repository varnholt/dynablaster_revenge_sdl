#pragma once

#include "framework/globaltime.h"

/// \brief SDL_GetTicks() backed GlobalTime, the clock animated materials and effects read via
/// GlobalTime::Instance().
class SdlGlobalTime : public GlobalTime
{
public:
   void update();
   float getTime() const override;

private:
   float _time = 0.0f;
};
