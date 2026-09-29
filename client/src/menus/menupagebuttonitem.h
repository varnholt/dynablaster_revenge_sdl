#pragma once

#include "menupageitem.h"

#include "framework/frametimer.h"

class MenuPageButtonItem : public MenuPageItem
{
public:
   MenuPageButtonItem();

   void draw() override;

   void setFocus(bool focus) override;

   void setEnabled(bool enabled) override;

private:
   FrameTimer _focus_out_time;

   float _fade_value = 0.0f;
   bool _fade_out = false;
};
