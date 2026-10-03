#include "menupagebackgrounditem.h"
#include "framework/gldevice.h"
#include "math/matrix.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
constexpr float COLOR_CHANGE_DURATION = 2000.0f;
}

MenuPageBackgroundItem::MenuPageBackgroundItem()
{
   _elapsed.start();
}

void MenuPageBackgroundItem::initialize()
{
   MenuPageItem::initialize();
}

void MenuPageBackgroundItem::addGradientLayer(PSDLayer& gradient, MenuPageBackgroundItem::BackgroundColor color)
{
   _background_layers[color] = gradient;
}

void MenuPageBackgroundItem::setBackgroundColor(MenuPageBackgroundItem::BackgroundColor color)
{
   if (color != _background_color)
   {
      _background_color_previous = _background_color;
      _background_color = color;
      _flip_background_elapsed.restart();
   }
}

void MenuPageBackgroundItem::draw()
{
   if (_background_layers[_background_color])
   {
      const float alpha = std::min(_flip_background_elapsed.elapsed() / COLOR_CHANGE_DURATION, 1.0f);
      const float alpha_inverted = 1.0f - alpha;

      if (alpha > 0.0f)
      {
         _background_layers[_background_color]->get().render(0.0f, 0.0f, alpha);
      }

      if (alpha_inverted > 0.0f)
      {
         _background_layers[_background_color_previous]->get().render(0.0f, 0.0f, alpha_inverted);
      }
   }

   _x = std::sin(_elapsed.elapsed() * 0.0001f);
   _y = std::cos(_elapsed.elapsed() * 0.0001f);

   // one quad for the whole background, scrolling slowly by animating its texcoords; rebuilt
   // into a small dynamic buffer every frame, drawn with the shader MenuDrawable already bound
   const PSD::Layer& psd_layer = getCurrentLayer()->get();

   const float height = static_cast<float>(psd_layer.getHeight());
   const float width = static_cast<float>(psd_layer.getWidth());
   const float x_translation = static_cast<float>(psd_layer.getLeft());
   const float y_translation = static_cast<float>(psd_layer.getTop());

   glBindTexture(GL_TEXTURE_2D, getActiveLayer()->get().getTexture());
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

   // clang-format off
   const std::array<float, 30> quad = {
      x_translation,         y_translation,          -1.0f, 0.0f + _x, 0.0f + _y,
      x_translation,         y_translation + height, -1.0f, 0.0f + _x, 1.0f + _y,
      x_translation + width, y_translation + height, -1.0f, 1.0f + _x, 1.0f + _y,
      x_translation,         y_translation,          -1.0f, 0.0f + _x, 0.0f + _y,
      x_translation + width, y_translation + height, -1.0f, 1.0f + _x, 1.0f + _y,
      x_translation + width, y_translation,          -1.0f, 1.0f + _x, 0.0f + _y,
   };
   // clang-format on
   constexpr int quad_size = static_cast<int>(sizeof(float) * 30);

   if (_vertex_buffer == 0)
   {
      _vertex_buffer = activeDevice().createVertexBuffer(quad_size, true);
   }
   else
   {
      activeDevice().allocateVertexBuffer(_vertex_buffer, quad_size, true);
   }

   std::ranges::copy(quad, activeDevice().lockVertexBuffer<float>(_vertex_buffer, quad_size).begin());
   activeDevice().unlockVertexBuffer(_vertex_buffer);

   // positions are already baked in page-pixel space, so the world transform must be identity
   activeDevice().push(Matrix());
   activeDevice().setParameter(activeDevice().getParameterIndex("alpha"), 1.0f);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<GLvoid*>(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   activeDevice().pop();
}
