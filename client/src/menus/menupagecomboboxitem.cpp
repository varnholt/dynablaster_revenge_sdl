#include "menupagecomboboxitem.h"

#include "clipper.h"
#include "framework/gldevice.h"
#include "math/matrix.h"
#include "menupagebuttonitem.h"
#include "menupagelabelitem.h"
#include "menupagelistitemelement.h"

#include <array>
#include <cstring>

MenuPageComboBoxItem::MenuPageComboBoxItem()
{
   _page_item_type = PageItemTypeCombobox;
}

void MenuPageComboBoxItem::initialize()
{
   MenuPageListItem::initialize();
   _clipper->setBounds(0, 0, 9999, 9999);

   // initially every combobox is invisible
   setVisible(false);

   _vertical_spacing = 0;
}

void MenuPageComboBoxItem::setFocus(bool focus)
{
   MenuPageListItem::setFocus(focus);

   if (!focus)
   {
      setVisible(false);
   }
}

bool MenuPageComboBoxItem::isModal() const
{
   return true;
}

void MenuPageComboBoxItem::setVisible(bool visible)
{
   if (visible != isVisible())
   {
      if (const auto button = getButtonItem())
      {
         button->get().setVisible(!visible);
      }

      if (const auto label = getLabelItem())
      {
         label->get().setVisible(!visible);
      }

      MenuPageListItem::setVisible(visible);
   }
}

void MenuPageComboBoxItem::drawQuad(const PSDLayer& layer, float x, float y, float width, float height, int opacity)
{
   glBindTexture(GL_TEXTURE_2D, layer.getTexture());

   const float u = layer.getU();
   const float v = layer.getV();

   // clang-format off
   const std::array<float, 30> quad = {
      x,         y,          0.0f, 0.0f, 0.0f,
      x,         y + height, 0.0f, 0.0f, v,
      x + width, y + height, 0.0f, u,    v,
      x,         y,          0.0f, 0.0f, 0.0f,
      x + width, y + height, 0.0f, u,    v,
      x + width, y,          0.0f, u,    0.0f,
   };
   // clang-format on
   constexpr int quad_size = static_cast<int>(sizeof(float) * 30);

   if (_quad_vertex_buffer == 0)
   {
      _quad_vertex_buffer = activeDevice().createVertexBuffer(quad_size, true);
   }
   else
   {
      activeDevice().allocateVertexBuffer(_quad_vertex_buffer, quad_size, true);
   }

   std::ranges::copy(quad, activeDevice().lockVertexBuffer<float>(_quad_vertex_buffer, quad_size).begin());
   activeDevice().unlockVertexBuffer(_quad_vertex_buffer);

   activeDevice().push(Matrix());
   activeDevice().setParameter(activeDevice().getParameterIndex("alpha"), opacity / 255.0f);

   glBindBuffer(GL_ARRAY_BUFFER, _quad_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<GLvoid*>(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   activeDevice().pop();
}

void MenuPageComboBoxItem::updateTableBounds()
{
   // call base to update maximum height
   MenuPageListItem::updateTableBounds();

   // update layer bounds
   PSD::Layer& layer = getCurrentLayer()->get();
   const int top = layer.getTop();
   const int max_height = getMaxTableHeight();

   layer.setBottom(top + max_height);
}

void MenuPageComboBoxItem::draw()
{
   if (!isVisible())
   {
      return;
   }

   const float y_step = _row_height + _vertical_spacing;
   const int element_count = static_cast<int>(_elements.size());
   float y_offset = 0.0f;

   const PSDLayer& first_element = getLayerFirstElement()->get();
   const PSDLayer& default_element = getLayerDefaultElement()->get();
   const PSDLayer& last_element = getLayerLastElement()->get();
   const PSDLayer& gradient_element = getLayerGradientElement()->get();

   // draw first element layer
   float height = _row_height;
   float width = first_element.getWidth();
   float x = first_element.getLeft();
   float y = first_element.getTop();

   drawQuad(first_element, x, y, width, height + 1);

   // draw n-1th element layer
   height = _row_height;
   width = default_element.getWidth();
   x = default_element.getLeft();
   y = first_element.getTop();

   for (int i = 1; i < element_count - 1; i++)
   {
      y_offset += y_step;
      drawQuad(default_element, x, y + y_offset, width, height);
   }

   // draw last element layer
   height = _row_height;
   width = last_element.getWidth();
   x = last_element.getLeft();
   y = first_element.getTop();

   y_offset += y_step;
   drawQuad(last_element, x, y + y_offset, width, height);

   // draw gradient
   height = element_count * _row_height;
   width = gradient_element.getWidth();
   x = gradient_element.getLeft();
   y = first_element.getTop();

   drawQuad(gradient_element, x, y, width, height, static_cast<int>(gradient_element.getOpacity() * 255.0f));

   MenuPageListItem::draw();
}

void MenuPageComboBoxItem::animate(float time)
{
   MenuPageListItem::animate(time);
}

void MenuPageComboBoxItem::dropDownEnabled(bool /*enabled*/)
{
}

void MenuPageComboBoxItem::setButtonItem(MenuPageButtonItem& item)
{
   _button_item = item;

   // both items belong to the same MenuPage and share its lifetime
   item.actionSignal.connect([this](const std::string&) { setVisible(true); });
}

std::optional<std::reference_wrapper<MenuPageButtonItem>> MenuPageComboBoxItem::getButtonItem() const
{
   return _button_item;
}

void MenuPageComboBoxItem::setLabelItem(MenuPageLabelItem& item)
{
   _label_item = item;

   valueChangedSignal.connect([&item](const std::string& value) { item.setText(value); });
}

std::optional<std::reference_wrapper<MenuPageLabelItem>> MenuPageComboBoxItem::getLabelItem() const
{
   return _label_item;
}

void MenuPageComboBoxItem::mousePressed(int x, int y)
{
   if (!isVisible())
   {
      return;
   }

   MenuPageListItem::mousePressed(x, y);

   if (static_cast<size_t>(_active_element) < _elements.size() && _elements.at(_active_element)->isActive())
   {
      setFocus(false);

      const std::string value = _elements.at(_active_element)->getText();
      valueChangedSignal(value);
   }
}

std::string MenuPageComboBoxItem::getValue() const
{
   if (const auto label = getLabelItem())
   {
      return label->get().getText();
   }

   return {};
}

void MenuPageComboBoxItem::setValue(const std::string& value)
{
   if (const auto label = getLabelItem())
   {
      label->get().setText(value);
   }
}
