#pragma once

#include "framework/frametimer.h"
#include "menupageitem.h"

#include "math/color.h"

#include <cstdint>
#include <functional>
#include <optional>

class BitmapFont;

/// \brief single-line text edit; the cursor highlight is a white quad drawn with a lazily
/// created 1x1 white texture (GLES3 has no untextured fixed-function quads).
class MenuPageTextEditItem : public MenuPageItem
{
public:
   MenuPageTextEditItem();

   //! draw textedit
   void draw() override;

   //! initialize textedit
   void initialize() override;

   //! getter for field width
   int getFieldWidth() const;

   //! setter for max length
   void setMaxLength(int max_length);

   //! getter for max length
   int getMaxLength() const;

   //! getter for scale
   float getScale() const;

   //! getter for text
   const std::string& getText() const;

   //! getter for color
   const Color& getColor() const;

   //! check if action request on click is enabled
   bool isActionRequestOnClickEnabled() const override;

   //! setter for cursor position
   void setCursorPosition(int);

   //! getter for cursor position
   int getCursorPosition() const;

   //! check if editing is active
   bool isEditingActive() const;

   void setText(const std::string&);

   void setScale(float scale);

   void setColor(const Color& color);

   void setOutlineColor(const Color& outline_color);

   void setAlpha(int alpha);

   void setFontName(const std::string& font_name);

   void setFontXOffset(int x_offset);

   void setFontYOffset(int y_offset);

   void setFieldWidth(int field_width);

   void keyPressed(int key, const std::string& text) override;

   void activated() override;

   void deactivated() override;

   void paste(const std::string& text) override;

protected:
   //! update the cursor's highlight
   void updateCursorHighlight();

   //! getter for cursor at end state
   bool isCursorAtEnd() const;

   //! cursor right
   void moveCursorRight();

   //! cursor left
   void moveCursorLeft();

   //! cursor to start
   void moveCursorToStart();

   //! cursor to end
   void moveCursorToEnd();

   //! draw the cursor
   void drawCursor();

   std::string _font_name;

   FrameTimer _timer;

   FrameTimer _cursor_time;

   // fonts belong to the FontPool
   std::optional<std::reference_wrapper<BitmapFont>> _font;

   std::string _text;

   int _font_x_offset = 0;
   int _font_y_offset = 0;
   int _field_width = 255;
   int _max_length = -1;

   float _scale = 0.0f;

   bool _editing_active = false;
   bool _cursor_visible = false;

   Color _color;

   int _alpha = 255;

   Color _outline_color;

   int _cursor_position = 0;

   // lazily created 1x1 white texture + dynamic quad buffer for drawCursor()
   uint32_t _cursor_texture = 0;
   uint32_t _cursor_vertex_buffer = 0;
};
