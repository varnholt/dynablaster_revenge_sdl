#pragma once

#include "signal.h"

class MenuPageAnimation
{
public:
   MenuPageAnimation() = default;
   virtual ~MenuPageAnimation() = default;

   virtual void initialize();

   virtual void start() = 0;

   virtual void animate() = 0;

   Signal<> stoppedSignal;
};
