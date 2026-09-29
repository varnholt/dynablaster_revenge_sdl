#pragma once

#include "menupageitem.h"

#include <memory>

class Clipper;

class MenuPageScrollImageItem : public MenuPageItem
{
public:
   MenuPageScrollImageItem();
   ~MenuPageScrollImageItem() override;

   void initialize() override;

   void draw() override;

   virtual void reset();

   void animate(float time) override;

protected:
   std::unique_ptr<Clipper> _clipper;

   float _y = 0.0f;
   float _start_time = 0.0f;
   float _animation_time = 0.0f;
   bool _move_up = false;
   float _relative_time_previous = 0.0f;
};
