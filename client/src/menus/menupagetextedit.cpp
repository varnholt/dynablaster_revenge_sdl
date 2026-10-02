#include "menupagetextedit.h"

#include "fontpool.h"
#include "framework/gldevice.h"
#include "math/matrix.h"

#include <SDL3/SDL_keycode.h>

#include <algorithm>
#include <array>
#include <cstring>

namespace
{
constexpr float CURSOR_UPDATE_TIME = 500.0f;
}

MenuPageTextEditItem::MenuPageTextEditItem()
{
   _page_item_type = PageItemTypeTextedit;
   _interactive = true;

   // the text is drawn, the layer only marks its position
   _layer_drawn = false;
}

void MenuPageTextEditItem::initialize()
{
   _font = FontPool::Instance()->get(_font_name.c_str());

   // init update timer
   _timer.setInterval(CURSOR_UPDATE_TIME);

   _timer.timeoutSignal.connect([this]() { updateCursorHighlight(); });
}

void MenuPageTextEditItem::draw()
{
   if (!isVisible())
   {
      return;
   }

   // optional background
   MenuPageItem::draw();

   if (_color.isValid())
   {
      _font->setColor(_color.redF(), _color.greenF(), _color.blueF(), _alpha / 255.0f);
   }
   else
   {
      _font->setColor(1.0f, 1.0f, 1.0f, _alpha / 255.0f);
   }

   //        i0               i0+maxLength
   //         [               ]
   //  [ABCDEFGHIJKLMNOPQRSTUVWXYZ]
   const int i0 = std::max(_cursor_position - getFieldWidth(), 0);
   const std::string visible_text = _text.substr(std::min(static_cast<size_t>(i0), _text.size()), getFieldWidth());

   _font->buildVertices(
      _scale, visible_text.c_str(), _layer_active->getLeft() + _font_x_offset, _layer_active->getBottom() + _font_y_offset
   );

   _font->draw();

   if (_editing_active)
   {
      drawCursor();
   }
}

void MenuPageTextEditItem::keyPressed(int key, const std::string& text)
{
   if (key == SDLK_BACKSPACE)
   {
      if (isCursorAtEnd())
      {
         // chop from end
         if (!_text.empty())
         {
            _text.pop_back();
         }
         moveCursorLeft();
      }
      else if (getCursorPosition() > 0)
      {
         // replace chars
         _text.erase(getCursorPosition() - 1, 1);
         moveCursorLeft();
      }
   }
   else if (key == SDLK_DELETE)
   {
      if (!isCursorAtEnd())
      {
         // replace chars
         _text.erase(getCursorPosition(), 1);
      }
   }
   else if (key == SDLK_LEFT)
   {
      moveCursorLeft();
   }
   else if (key == SDLK_RIGHT)
   {
      moveCursorRight();
   }
   else if (key == SDLK_HOME)
   {
      moveCursorToStart();
   }
   else if (key == SDLK_END)
   {
      moveCursorToEnd();
   }
   else if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_ESCAPE)
   {
      // ignored
   }
   else if (!text.empty())
   {
      if (isCursorAtEnd())
      {
         // append chars
         if (static_cast<int>(_text.length()) < getMaxLength())
         {
            _text.append(text);
         }
      }
      else
      {
         // replace chars
         _text.replace(getCursorPosition(), 1, text);
      }

      moveCursorRight();
   }
}

void MenuPageTextEditItem::setFontName(const std::string& font_name)
{
   _font_name = font_name;
}

void MenuPageTextEditItem::setFontXOffset(int x_offset)
{
   _font_x_offset = x_offset;
}

void MenuPageTextEditItem::setFontYOffset(int y_offset)
{
   _font_y_offset = y_offset;
}

void MenuPageTextEditItem::setFieldWidth(int field_width)
{
   _field_width = field_width;
}

void MenuPageTextEditItem::setMaxLength(int max_length)
{
   _max_length = max_length;
}

int MenuPageTextEditItem::getMaxLength() const
{
   return _max_length;
}

int MenuPageTextEditItem::getFieldWidth() const
{
   return _field_width;
}

void MenuPageTextEditItem::setScale(float scale)
{
   _scale = scale;
}

float MenuPageTextEditItem::getScale() const
{
   return _scale;
}

void MenuPageTextEditItem::setText(const std::string& text)
{
   _text = text.substr(0, std::max(text.length(), static_cast<size_t>(getFieldWidth())));
   setCursorPosition(static_cast<int>(_text.length()));
}

void MenuPageTextEditItem::setColor(const Color& color)
{
   _color = color;
}

void MenuPageTextEditItem::setOutlineColor(const Color& outline_color)
{
   _outline_color = outline_color;
}

const Color& MenuPageTextEditItem::getColor() const
{
   return _color;
}

void MenuPageTextEditItem::setAlpha(int alpha)
{
   _alpha = alpha;
}

bool MenuPageTextEditItem::isActionRequestOnClickEnabled() const
{
   return false;
}

void MenuPageTextEditItem::setCursorPosition(int index)
{
   _cursor_position = index;
}

int MenuPageTextEditItem::getCursorPosition() const
{
   return _cursor_position;
}

bool MenuPageTextEditItem::isEditingActive() const
{
   return _editing_active;
}

void MenuPageTextEditItem::moveCursorRight()
{
   setCursorPosition(std::min(getCursorPosition() + 1, static_cast<int>(getText().length())));
}

void MenuPageTextEditItem::moveCursorLeft()
{
   setCursorPosition(std::max(getCursorPosition() - 1, 0));
}

void MenuPageTextEditItem::moveCursorToStart()
{
   setCursorPosition(0);
}

void MenuPageTextEditItem::moveCursorToEnd()
{
   setCursorPosition(static_cast<int>(getText().length()));
}

const std::string& MenuPageTextEditItem::getText() const
{
   return _text;
}

void MenuPageTextEditItem::activated()
{
   _timer.start();

   // call the timer's slot once initially
   updateCursorHighlight();

   _editing_active = true;
   MenuPageItem::activated();
}

void MenuPageTextEditItem::deactivated()
{
   _timer.stop();
   _editing_active = false;
   MenuPageItem::deactivated();
}

void MenuPageTextEditItem::paste(const std::string& text)
{
   for (const char c : text)
   {
      keyPressed(SDLK_UNKNOWN, std::string(1, c));
   }
}

void MenuPageTextEditItem::drawCursor()
{
   const float alpha_factor = std::max(1.0f - 0.75f * (_cursor_time.elapsed() / CURSOR_UPDATE_TIME), 0.0f);

   float left = 0.0f;
   float right = 0.0f;
   float top = 0.0f;
   float bottom = 0.0f;

   _font->getCursor(_scale, getCursorPosition(), left, right, top, bottom);

   if (_cursor_texture == 0)
   {
      uint32_t white = 0xFFFFFFFF;
      _cursor_texture = activeDevice->createTexture(&white, 1, 1, 0);
   }

   glBindTexture(GL_TEXTURE_2D, _cursor_texture);

   glBlendFunc(GL_SRC_ALPHA, GL_SRC_COLOR);

   // clang-format off
   const std::array<float, 30> quad = {
      left,  top,    -1.0f, 0.0f, 0.0f,
      right, top,    -1.0f, 1.0f, 0.0f,
      right, bottom, -1.0f, 1.0f, 1.0f,
      left,  top,    -1.0f, 0.0f, 0.0f,
      right, bottom, -1.0f, 1.0f, 1.0f,
      left,  bottom, -1.0f, 0.0f, 1.0f,
   };
   // clang-format on
   constexpr int quad_size = static_cast<int>(sizeof(float) * 30);

   if (_cursor_vertex_buffer == 0)
   {
      _cursor_vertex_buffer = activeDevice->createVertexBuffer(quad_size, true);
   }
   else
   {
      activeDevice->allocateVertexBuffer(_cursor_vertex_buffer, quad_size, true);
   }

   void* destination = activeDevice->lockVertexBuffer(_cursor_vertex_buffer, quad_size);
   std::memcpy(destination, quad.data(), quad_size);
   activeDevice->unlockVertexBuffer(_cursor_vertex_buffer);

   activeDevice->push(Matrix());
   activeDevice->setParameter(activeDevice->getParameterIndex("alpha"), (128.0f / 255.0f) * alpha_factor);

   glBindBuffer(GL_ARRAY_BUFFER, _cursor_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<GLvoid*>(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   activeDevice->pop();

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void MenuPageTextEditItem::updateCursorHighlight()
{
   _cursor_time.restart();
}

bool MenuPageTextEditItem::isCursorAtEnd() const
{
   return getCursorPosition() == static_cast<int>(getText().length());
}
