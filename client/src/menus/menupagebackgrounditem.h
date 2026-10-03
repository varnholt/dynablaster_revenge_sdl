#pragma once

#include "menupageitem.h"

#include "framework/frametimer.h"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>

class MenuPageBackgroundItem : public MenuPageItem
{
public:
   enum BackgroundColor
   {
      BackgroundColorRed = 0,
      BackgroundColorGreen,
      BackgroundColorBlue
   };

   MenuPageBackgroundItem();

   void draw() override;

   void initialize() override;

   void addGradientLayer(PSDLayer& gradient, BackgroundColor color);

   void setBackgroundColor(BackgroundColor);

private:
   FrameTimer _elapsed;

   float _x = 0.0f;
   float _y = 0.0f;

   FrameTimer _flip_background_elapsed;
   BackgroundColor _background_color = BackgroundColorBlue;
   BackgroundColor _background_color_previous = BackgroundColorBlue;

   // the layers belong to the MenuPage
   std::array<std::optional<std::reference_wrapper<PSDLayer>>, BackgroundColorBlue + 1> _background_layers{};

   // the scrolling quad drawn directly (its texcoords animate every frame) - see draw()
   uint32_t _vertex_buffer = 0;
};
