#pragma once

#include "menupageitem.h"
#include "gamesignal.h"

class MenuPageSliderItem : public MenuPageItem
{
public:
   MenuPageSliderItem();

   bool isGrabbingMouseEvents() override;

   void mousePressed(int x, int y) override;

   void mouseMoved(int x, int y) override;

   void mouseReleased() override;

   void setMinimum(int minimum);
   void setMaximum(int maximum);

   int getMinimum() const;
   int getMaximum() const;

   float getValue() const;

   void setValue(float);

   Signal<float> valueChangedSignal;

protected:
   int _minimum = 0;
   int _maximum = 0;

   float _value = 0.0f;

   int _relative_to_x = 0;
};
