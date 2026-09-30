#pragma once

#include "framework/frametimer.h"
#include "menupageitem.h"
#include "gamesignal.h"

#include "math/color.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class Clipper;
class MenuPageListItemElement;

/// \brief scrollable list of text rows; rows are drawn with the listhighlight shader as small
/// dynamic vertex buffers (pos + two texcoord sets), rebuilt once per row per frame.
class MenuPageListItem : public MenuPageItem
{
public:
   MenuPageListItem();
   ~MenuPageListItem() override;

   void initialize() override;

   void draw() override;

   void setFontName(const std::string& font_name);

   void setFontXOffset(int x_offset);

   void setFontYOffset(int y_offset);

   void setFieldWidth(int field_width);

   void setScale(float scale);

   void setRowHeight(int height);

   //! getter for element at i
   MenuPageListItemElement* getElementAt(int i) const;

   //! get element text
   const std::string& getElementText(int element);

   //! getter for element count
   int getElementCount();

   //! getter for the active element id
   int getActiveElement() const;

   //! setter for active element id
   void setActiveElement(int);

   //! setter for the active element id
   void setElementActive(int, bool);

   //! getter for focussed element id
   int getFocussedElement() const;

   //! setter for focussed element id
   void setFocussedElement(int element);

   //! setter for element focus
   void setElementFocussed(int element, bool focussed);

   //! append an item to the list
   virtual void appendItem(
      const std::string& item,
      const Color& color = Color("#FFFFFF"),
      bool override_alpha = false,
      const Color& outline_color = Color()
   );

   //! clear all items from the list
   void clear();

   //! set list highlighting enabled
   void setHighlightingEnabled(bool enabled);

   //! getter for highlighting flag
   bool isHighlightingEnabled() const;

   //! set row alphas
   void setRowAlphas(int row0, int row1);

   bool hasNestedElements() override;

   void mouseMoved(int x, int y) override;

   void mousePressed(int x, int y) override;

   virtual void setLayerFirstElement(PSDLayer* layer);
   virtual void setLayerDefaultElement(PSDLayer* layer);
   virtual void setLayerLastElement(PSDLayer* layer);
   virtual void setLayerGradientElement(PSDLayer* layer);
   virtual void setLayerSelectedElement(PSDLayer* layer);
   virtual void setLayerFocussedElement(PSDLayer* layer);

   virtual PSDLayer* getLayerFirstElement() const;
   virtual PSDLayer* getLayerDefaultElement() const;
   virtual PSDLayer* getLayerLastElement() const;
   virtual PSDLayer* getLayerGradientElement() const;
   virtual PSDLayer* getLayerSelectedElement() const;
   virtual PSDLayer* getLayerFocussedElement() const;

   //! getter for blend duration
   float getBlendDuration() const;

   //! setter for blend duration
   void setBlendDuration(float value);

   //! getter for y offset source
   float getYOffsetSource() const;

   //! setter for y offset source
   void setYOffsetSource(float value);

   //! getter for y offset dest
   float getYOffsetDest() const;

   //! setter for y offset dest
   void setYOffsetDest(float value);

   void animate(float time) override;

   virtual void scrollUp();

   virtual void scrollDown();

   virtual void scrollStop();

   //! scroll to particular percentage of table
   virtual void scrollToPercentage(float percent, bool clicked = true);

   //! scroll to item of given index
   virtual void scrollToIndex(int index, bool clicked = true);

   //! smooth scroll to index
   virtual int scrollSmoothToIndex(int index);

   Signal<float> scrollAnimationSignal;

   Signal<int> elementFocussedSignal;

protected:
   //! draw the text
   void drawText();

   //! draw the rows
   void drawRows();

   //! bind list item shader
   void bindShader();

   //! release list item shader
   void releaseShader();

   //! bind row texture (if present)
   PSDLayer* bindRowTexture(int row, float& u, float& v, float& s, float& t);

   //! generate a new item instance
   std::unique_ptr<MenuPageListItemElement> itemInstance();

   //! initialize item instance
   void initializeItem(MenuPageListItemElement* element, int index);

   //! update table bounds
   virtual void updateTableBounds();

   //! getter for maximum table height
   virtual int getMaxTableHeight() const;

   //! getter for maximum table width
   virtual int getMaxTableWidth() const;

   //! set alpha value for given element
   void selectAlpha(int row_toggle, MenuPageListItemElement* element);

   //! update scrollbars depending on current offset
   void updateScrollbars();

   //! limit y to not allow table movement out of bounds
   void limitY(float& y);

   //! update focussed element from given relative y position
   void updateFocussedElement(int relative_y);

   //! clipper to clip table to
   std::unique_ptr<Clipper> _clipper;

   std::vector<std::unique_ptr<MenuPageListItemElement>> _elements;

   //! time elapsed used for scrolling animation
   FrameTimer _elapsed;

   // offset
   float _x = 0.0f;
   float _y = 0.0f;

   // dimensions
   float _width_all_elements = 0.0f;
   float _height_all_elements = 0.0f;

   // properties for single lineedits
   std::string _font_name;
   int _font_x_offset = 0;
   int _font_y_offset = 0;
   int _field_width = 255;
   float _scale = 0.0f;

   float _scroll_value = 0.0f;
   int _vertical_spacing = 0;
   int _row_height = 0;
   int _focussed_element = 0;
   int _active_element = 0;
   bool _scrolling_active = false;
   bool _highlighting_active = true;

   // individual layers for elements (to be used by comboboxes etc), non-owning
   PSDLayer* _layer_first_element = nullptr;
   PSDLayer* _layer_default_element = nullptr;
   PSDLayer* _layer_last_element = nullptr;
   PSDLayer* _layer_gradient = nullptr;
   PSDLayer* _layer_selected_element = nullptr;
   PSDLayer* _layer_focussed_element = nullptr;

   //! listhighlight shader and its uniform locations
   uint32_t _shader = 0;
   int _param_texture_clamp = 0;
   int _param_texture_highlight = 0;
   int _param_row_alpha = 0;

   //! per-row dynamic vertex buffer (pos + uvClamp + uvHighlight), rebuilt every draw
   uint32_t _row_vertex_buffer = 0;

   std::array<int, 2> _row_alpha = {15, 20};

   // blend between two positions
   FrameTimer _blend_timer;
   float _blend_duration = 0.0f;
   float _y_offset_source = 0.0f;
   float _y_offset_destination = 0.0f;

   //! copy of destination
   float _y_destination = 0.0f;

   //! last mouse y position
   int _mouse_y = 0;
};
