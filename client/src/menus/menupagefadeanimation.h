#pragma once

#include "framework/frametimer.h"
#include "menupageanimation.h"

class MenuPageFadeAnimation : public MenuPageAnimation
{
public:
   void setAlpha(float);

   void setFadeIn(bool);

   void initialize() override;

   float getAlpha() const;

   bool isStopped() const;

   void setStopped(bool value);

   void start() override;

   void animate() override;

private:
   FrameTimer _elapsed;

   bool _stopped = false;
   bool _fade_in = false;
   float _alpha = 0.0f;
};
