#pragma once

#include "menupageitem.h"
#include "signal.h"

class MenuPageScrollbar : public MenuPageItem
{
public:
   MenuPageScrollbar();

   bool isGrabbingMouseEvents() override;

   void mousePressed(int x, int y) override;

   void mouseMoved(int x, int y) override;

   void mouseReleased() override;

   void setHeight(int);

   void setTop(int);

   void updateFromAnimation(float);

   Signal<float> scrollToPercentageSignal;

protected:
   int _position = 0;
   int _height = 0;
   int _top = 0;
   float _offset = 0.0f;
   int _relative_to_y = 0;

   //! suppresses scrollToPercentageSignal while updateFromAnimation() drives mouseMoved()
   //! programmatically, to avoid feeding the table's own scrollAnimation update straight back to it
   bool _signals_blocked = false;
};
