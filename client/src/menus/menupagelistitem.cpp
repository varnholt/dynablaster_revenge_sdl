#include "menupagelistitem.h"

#include "clipper.h"
#include "defaultshader.h"
#include "menupagelistitemelement.h"

#include "framework/gldevice.h"
#include "math/matrix.h"

#include "logging.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace
{
constexpr float SCROLL_SPEED = 5.0f;
}

MenuPageListItem::MenuPageListItem()
{
   _page_item_type = PageItemTypeList;
   _interactive = true;
}

MenuPageListItem::~MenuPageListItem() = default;

std::unique_ptr<MenuPageListItemElement> MenuPageListItem::itemInstance()
{
   return std::make_unique<MenuPageListItemElement>(*this);
}

void MenuPageListItem::initializeItem(MenuPageListItemElement& element, int index)
{
   if (_layer_active)
   {
      // set element properties
      element.setIndex(index);
      element.setHeight(_row_height);
      element.setWidth(_layer_active->get().getWidth());
      element.setX(0);
      element.setY(index * _row_height + index * _vertical_spacing);

      // lineedit properties
      element.setFontXOffset(_font_x_offset);
      element.setFontYOffset(_font_y_offset);
      element.setFieldWidth(_field_width);
      element.setScale(_scale);
   }
   else
   {
      qWarning(
         "MenuPageListItem::initializeItem: you can't initialize list items elements "
         "without initializing the pagelist item"
      );
   }
}

void MenuPageListItem::clear()
{
   _elements.clear();

   // reinit table bounds
   updateTableBounds();
}

void MenuPageListItem::setHighlightingEnabled(bool enabled)
{
   _highlighting_active = enabled;
}

bool MenuPageListItem::isHighlightingEnabled() const
{
   return _highlighting_active;
}

void MenuPageListItem::appendItem(const std::string& text, const Color& color, bool override_alpha, const Color& outline_color)
{
   auto element = itemInstance();

   // set text and color
   element->setFontName(_font_name);
   element->setText(text);
   element->setColor(color);
   element->setOverrideAlpha(override_alpha);

   if (outline_color.isValid())
   {
      element->setOutlineColor(outline_color);
   }

   // set element properties
   initializeItem(*element, static_cast<int>(_elements.size()));

   // generate element's vertices
   element->initialize();

   _elements.push_back(std::move(element));

   // reinit table bounds
   updateTableBounds();
}

void MenuPageListItem::updateTableBounds()
{
   const int count = static_cast<int>(_elements.size());

   // pixels per element + pixels between elements
   _height_all_elements = (count * _row_height) + (count - 1) * _vertical_spacing;
}

int MenuPageListItem::getMaxTableHeight() const
{
   return _height_all_elements;
}

int MenuPageListItem::getMaxTableWidth() const
{
   return _width_all_elements;
}

void MenuPageListItem::initialize()
{
   _vertical_spacing = 3;

   _clipper = std::make_unique<Clipper>(
      static_cast<float>(_layer_active->get().getLeft()),
      static_cast<float>(_layer_active->get().getTop()),
      static_cast<float>(_layer_active->get().getLeft() + _layer_active->get().getWidth()),
      static_cast<float>(_layer_active->get().getTop() + _layer_active->get().getHeight())
   );

   _elapsed.start();

   _shader = activeDevice->loadShader("data/shaders/listhighlight-vert.glsl", "data/shaders/listhighlight-frag.glsl");
   _param_texture_clamp = activeDevice->getParameterIndex("textureClamp");
   _param_texture_highlight = activeDevice->getParameterIndex("textureHighlight");
   _param_row_alpha = activeDevice->getParameterIndex("rowAlpha");
}

void MenuPageListItem::updateScrollbars()
{
   const float percent = _y / (-_height_all_elements + _layer_active->get().getHeight());
   scrollAnimationSignal(percent);
}

void MenuPageListItem::limitY(float& y)
{
   if (y > 0.0f)
   {
      y = 0.0f;
   }
   else if (_height_all_elements + y < _layer_active->get().getHeight() && _height_all_elements >= _layer_active->get().getHeight())
   {
      y = _layer_active->get().getHeight() - _height_all_elements;
   }
}

void MenuPageListItem::animate(float /*time*/)
{
   // simple mouse-triggered scrolling animation
   const float deceleration = 1.0f + std::sin(_elapsed.elapsed() * 0.01f);
   const float scroll_value_moving = _scroll_value * SCROLL_SPEED;
   const float scroll_value_stopping = deceleration * 0.5f * scroll_value_moving;

   const float value = _scrolling_active ? scroll_value_moving : scroll_value_stopping;

   _y += value;

   // smooth scrolling animation to target position
   const float duration = getBlendDuration() * 1000.0f;
   if (_blend_timer.elapsed() < duration)
   {
      const float elapsed = _blend_timer.elapsed();

      const float a = 0.5f * (1.0f + std::cos(std::numbers::pi_v<float> * (elapsed / duration)));
      const float b = 1.0f - a;

      _y = a * getYOffsetSource() + b * getYOffsetDest();

      updateScrollbars();

      // update the focussed element from the mouse position so the correct element is highlighted
      int relative_y = _mouse_y - _layer_active->get().getTop();
      relative_y -= _y_destination;
      updateFocussedElement(relative_y);
   }

   limitY(_y);

   // scroll animation
   if ((value < 0.1f && value > 0) || (value > -0.1f && value < 0))
   {
      _scroll_value = 0.0f;
   }
   else if (_scroll_value != 0.0f)
   {
      updateScrollbars();
   }
}

void MenuPageListItem::selectAlpha(int row_toggle, MenuPageListItemElement& element)
{
   float alpha = 0.0f;

   if (_highlighting_active)
   {
      if (element.isFadingOut())
      {
         alpha = (_row_alpha[row_toggle] + 30 * element.getFadeOutValue()) / 255.0f;
      }
      else if (element.isFocussed() || element.isActive())
      {
         alpha = (_row_alpha[row_toggle] + 30) / 255.0f;
      }
      else
      {
         alpha = _row_alpha[row_toggle] / 255.0f;
      }
   }
   else
   {
      alpha = _row_alpha[0] / 255.0f;
   }

   activeDevice->setParameter(_param_row_alpha, alpha);
}

float MenuPageListItem::getYOffsetDest() const
{
   return _y_offset_destination;
}

void MenuPageListItem::setYOffsetDest(float value)
{
   _y_offset_destination = value;
}

float MenuPageListItem::getYOffsetSource() const
{
   return _y_offset_source;
}

void MenuPageListItem::setYOffsetSource(float value)
{
   _y_offset_source = value;
}

float MenuPageListItem::getBlendDuration() const
{
   return _blend_duration;
}

void MenuPageListItem::setBlendDuration(float value)
{
   _blend_duration = value;
}

void MenuPageListItem::setRowAlphas(int row0, int row1)
{
   _row_alpha = {row0, row1};
}

void MenuPageListItem::bindShader()
{
   if (getLayerFirstElement() || getLayerLastElement())
   {
      activeDevice->setShader(_shader);

      activeDevice->bindSampler(_param_texture_clamp, 0);
      activeDevice->bindSampler(_param_texture_highlight, 1);
   }
}

void MenuPageListItem::releaseShader()
{
   if (getLayerFirstElement() || getLayerLastElement())
   {
      // restore the shared menu shader rather than "no shader" - see defaultshader.h
      activeDevice->setShader(getDefaultMenuShader());
   }
}

void MenuPageListItem::bindRowTexture(int row, float& u, float& v, float& s, float& t)
{
   std::optional<std::reference_wrapper<PSDLayer>> layer;
   const int last_row = static_cast<int>(_elements.size()) - 1;

   if (row == 0 && getLayerFirstElement())
   {
      layer = getLayerFirstElement();
   }
   else if (row == last_row && getLayerLastElement())
   {
      layer = getLayerLastElement();
   }
   else if (row > 0 && row < last_row && getLayerDefaultElement())
   {
      layer = getLayerDefaultElement();
   }

   glActiveTexture(GL_TEXTURE0);

   if (layer)
   {
      glBindTexture(GL_TEXTURE_2D, layer->get().getTexture());
      u = layer->get().getU();
      v = layer->get().getV();
   }
   else
   {
      glBindTexture(GL_TEXTURE_2D, 0);
      u = 0.0f;
      v = 0.0f;
   }

   // set selected element textures
   glActiveTexture(GL_TEXTURE1);

   if (const auto selected = getLayerSelectedElement())
   {
      glBindTexture(GL_TEXTURE_2D, selected->get().getTexture());
      s = selected->get().getU();
      t = selected->get().getV();
   }
   else
   {
      glBindTexture(GL_TEXTURE_2D, 0);
      s = 0.0f;
      t = 0.0f;
   }

   glActiveTexture(GL_TEXTURE0);
}

void MenuPageListItem::drawText()
{
   for (const auto& element : _elements)
   {
      const float opacity = (element->isFocussed() || element->isActive() || element->isOverrideAlphaActive()) ? 1.0f : 0.5882f;

      std::vector<Vertex> bound = element->getBoundingRectVertices(_layer_active->get().getLeft(), _layer_active->get().getTop() + _y);

      if (_clipper->enable(bound))
      {
         element->draw(_layer_active->get().getLeft(), _layer_active->get().getTop() + _y, opacity);

         _clipper->disable();
      }
   }
}

// the bounding rect's 4-vertex loop (see MenuPageListItemElement::getBoundingRectVertices) is
// triangulated as (0,1,2)/(0,2,3)
void MenuPageListItem::drawRows()
{
   // a plain (non-combobox) list never gets first/last/default row-background layers; GLES3 has
   // no fixed-function fallback, so drawing would put garbage-shaded quads over every row
   if (!getLayerFirstElement() && !getLayerLastElement())
   {
      return;
   }

   bindShader();

   constexpr std::array<int, 6> order = {0, 1, 2, 0, 2, 3};
   constexpr int floats_per_vertex = 7;

   int row_toggle = 0;
   int row = 0;
   for (const auto& element : _elements)
   {
      std::vector<Vertex> bounding_rect =
         element->getBoundingRectVertices(_layer_active->get().getLeft(), _layer_active->get().getTop() + _y);

      if (_clipper->enable(bounding_rect))
      {
         float u = 0.0f;
         float v = 0.0f;
         float s = 0.0f;
         float t = 0.0f;
         bindRowTexture(row, u, v, s, t);

         selectAlpha(row_toggle, *element);

         std::array<float, order.size() * floats_per_vertex> quad{};

         for (size_t i = 0; i < order.size(); i++)
         {
            const Vertex& vertex = bounding_rect[order[i]];
            const size_t offset = i * floats_per_vertex;
            quad[offset + 0] = vertex.x;
            quad[offset + 1] = vertex.y;
            quad[offset + 2] = 0.0f;
            quad[offset + 3] = vertex.u * u;
            quad[offset + 4] = vertex.v * v;
            quad[offset + 5] = vertex.u * s;
            quad[offset + 6] = vertex.v * t;
         }

         const int quad_size = static_cast<int>(sizeof(float) * quad.size());

         if (_row_vertex_buffer == 0)
         {
            _row_vertex_buffer = activeDevice->createVertexBuffer(quad_size, true);
         }
         else
         {
            activeDevice->allocateVertexBuffer(_row_vertex_buffer, quad_size, true);
         }

         std::memcpy(activeDevice->lockVertexBuffer(_row_vertex_buffer, quad_size), quad.data(), quad_size);
         activeDevice->unlockVertexBuffer(_row_vertex_buffer);

         activeDevice->push(Matrix());

         glBindBuffer(GL_ARRAY_BUFFER, _row_vertex_buffer);
         glEnableVertexAttribArray(0);
         glEnableVertexAttribArray(1);
         glEnableVertexAttribArray(2);
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, nullptr);
         glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 7, reinterpret_cast<GLvoid*>(sizeof(float) * 3));
         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 7, reinterpret_cast<GLvoid*>(sizeof(float) * 5));

         glDrawArrays(GL_TRIANGLES, 0, 6);

         glDisableVertexAttribArray(0);
         glDisableVertexAttribArray(1);
         glDisableVertexAttribArray(2);

         activeDevice->pop();

         if (element->isFadingOut() && element->getFadeOutValue() < 0.1f)
         {
            element->stopFadeOut();
         }

         _clipper->disable();
      }

      row_toggle ^= 1;
      row++;
   }

   releaseShader();
}

void MenuPageListItem::draw()
{
   drawText();
   drawRows();

   // release texture 1
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
}

void MenuPageListItem::setFontName(const std::string& font_name)
{
   _font_name = font_name;
}

void MenuPageListItem::setFontXOffset(int x_offset)
{
   _font_x_offset = x_offset;
}

void MenuPageListItem::setFontYOffset(int y_offset)
{
   _font_y_offset = y_offset;
}

void MenuPageListItem::setFieldWidth(int field_width)
{
   _field_width = field_width;
}

void MenuPageListItem::setScale(float scale)
{
   _scale = scale;
}

void MenuPageListItem::setRowHeight(int height)
{
   _row_height = height;
}

void MenuPageListItem::scrollToIndex(int /*index*/, bool /*clicked*/)
{
}

int MenuPageListItem::scrollSmoothToIndex(int index)
{
   // autocorrect input
   const int element_count = getElementCount();
   if (index < 0)
   {
      index = 0;
   }
   if (index > element_count - 1)
   {
      index = element_count - 1;
   }

   // init distance and target position
   const float one_row_height = _row_height + _vertical_spacing;
   const float table_vertical_center = _layer_active->get().getHeight() * 0.5f;

   const float destination = table_vertical_center - _height_all_elements + ((element_count - index) * one_row_height);

   // init animation
   setYOffsetSource(_y);
   setYOffsetDest(destination);

   const float distance = std::abs(getYOffsetSource() - getYOffsetDest()) / _height_all_elements;
   const float duration = 2.0f * distance;

   setBlendDuration(duration);
   _blend_timer.restart();

   // compute mouse cursor position
   _y_destination = destination;
   limitY(_y_destination);

   const int table_top = _layer_active->get().getTop();
   const int table_height = _layer_active->get().getHeight();

   const int row_height = one_row_height * index;
   int mouse_offset = 0;

   if (static_cast<int>(_y_destination) == 0)
   {
      // at top: initialize mouse offset with the (index * row height)
      mouse_offset = row_height;
   }
   else if (static_cast<int>(_y_destination) == -static_cast<int>(_height_all_elements - table_height))
   {
      // bottom reached
      mouse_offset = table_height - (_height_all_elements - row_height);
   }
   else
   {
      // in between
      mouse_offset = table_vertical_center;
   }

   return table_top + mouse_offset;
}

void MenuPageListItem::scrollToPercentage(float percent, bool clicked)
{
   _y = -_height_all_elements + _layer_active->get().getHeight();
   _y *= percent;

   if (!clicked)
   {
      scrollAnimationSignal(percent);
   }
}

void MenuPageListItem::scrollUp()
{
   _scroll_value = 1.0f;
   _scrolling_active = true;
}

void MenuPageListItem::scrollDown()
{
   _scroll_value = -1.0f;
   _scrolling_active = true;
}

void MenuPageListItem::scrollStop()
{
   _elapsed.restart();
   _scrolling_active = false;
}

bool MenuPageListItem::hasNestedElements()
{
   return true;
}

void MenuPageListItem::updateFocussedElement(int relative_y)
{
   const int focussed_element = static_cast<float>(relative_y) / (_vertical_spacing + _row_height);

   if (focussed_element > -1 && static_cast<int>(_elements.size()) > focussed_element)
   {
      _elements.at(focussed_element)->setFocus(true);

      // only one element can have focus
      if (focussed_element != _focussed_element && static_cast<size_t>(_focussed_element) < _elements.size())
      {
         _elements.at(_focussed_element)->setFocus(false);
      }

      setFocussedElement(focussed_element);
   }
}

void MenuPageListItem::mouseMoved(int /*x*/, int y)
{
   if (isVisible())
   {
      // store last mouse position
      _mouse_y = y;

      int relative_y = y - _layer_active->get().getTop();
      relative_y -= _y;

      updateFocussedElement(relative_y);
   }
}

void MenuPageListItem::mousePressed(int /*x*/, int y)
{
   int relative_y = y - _layer_active->get().getTop();
   relative_y -= _y;

   const int active_element = static_cast<float>(relative_y) / (_vertical_spacing + _row_height);

   if (active_element > -1 && static_cast<int>(_elements.size()) > active_element)
   {
      _elements.at(active_element)->setActive(true);

      // only one element can have focus
      if (active_element != _active_element && static_cast<size_t>(_active_element) < _elements.size())
      {
         _elements.at(_active_element)->setActive(false);
      }

      setActiveElement(active_element);
   }
}

std::optional<std::reference_wrapper<MenuPageListItemElement>> MenuPageListItem::getElementAt(int i) const
{
   if (i < 0 || static_cast<size_t>(i) >= _elements.size())
   {
      return std::nullopt;
   }
   return *_elements[static_cast<size_t>(i)];
}

const std::string& MenuPageListItem::getElementText(int element)
{
   return _elements.at(element)->getText();
}

int MenuPageListItem::getElementCount()
{
   return static_cast<int>(_elements.size());
}

int MenuPageListItem::getActiveElement() const
{
   return _active_element;
}

void MenuPageListItem::setActiveElement(int element)
{
   _active_element = element;
}

void MenuPageListItem::setElementActive(int element, bool active)
{
   if (static_cast<size_t>(element) < _elements.size())
   {
      _elements.at(element)->setActive(active);
   }
}

int MenuPageListItem::getFocussedElement() const
{
   return _focussed_element;
}

void MenuPageListItem::setFocussedElement(int element)
{
   _focussed_element = element;

   elementFocussedSignal(element);
}

void MenuPageListItem::setElementFocussed(int element, bool focussed)
{
   if (static_cast<size_t>(element) < _elements.size())
   {
      _elements.at(element)->setFocus(focussed);
   }
}

void MenuPageListItem::setLayerFirstElement(PSDLayer& layer)
{
   _layer_first_element = layer;
}

void MenuPageListItem::setLayerDefaultElement(PSDLayer& layer)
{
   _layer_default_element = layer;
}

void MenuPageListItem::setLayerLastElement(PSDLayer& layer)
{
   _layer_last_element = layer;
}

void MenuPageListItem::setLayerGradientElement(PSDLayer& layer)
{
   _layer_gradient = layer;
}

void MenuPageListItem::setLayerSelectedElement(PSDLayer& layer)
{
   _layer_selected_element = layer;
}

void MenuPageListItem::setLayerFocussedElement(PSDLayer& layer)
{
   _layer_focussed_element = layer;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageListItem::getLayerFirstElement() const
{
   return _layer_first_element;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageListItem::getLayerDefaultElement() const
{
   return _layer_default_element;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageListItem::getLayerLastElement() const
{
   return _layer_last_element;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageListItem::getLayerGradientElement() const
{
   return _layer_gradient;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageListItem::getLayerSelectedElement() const
{
   return _layer_selected_element;
}

std::optional<std::reference_wrapper<PSDLayer>> MenuPageListItem::getLayerFocussedElement() const
{
   return _layer_focussed_element;
}
