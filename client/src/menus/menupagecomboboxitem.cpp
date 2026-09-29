#include "menupagecomboboxitem.h"

#include "clipper.h"
#include "framework/gldevice.h"
#include "math/matrix.h"
#include "menupagebuttonitem.h"
#include "menupagelabelitem.h"
#include "menupagelistitemelement.h"

#include <array>
#include <cstring>

std::map<std::string, MenuPageComboBoxItem*> MenuPageComboBoxItem::_map_combo_boxes;
std::map<std::string, MenuPageLabelItem*> MenuPageComboBoxItem::_map_labels;
std::map<std::string, MenuPageButtonItem*> MenuPageComboBoxItem::_map_buttons;

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
      if (MenuPageButtonItem* button = getButtonItem())
      {
         button->setVisible(!visible);
      }

      if (MenuPageLabelItem* label = getLabelItem())
      {
         label->setVisible(!visible);
      }

      MenuPageListItem::setVisible(visible);
   }
}

void MenuPageComboBoxItem::drawQuad(PSDLayer* layer, float x, float y, float width, float height, int opacity)
{
   glBindTexture(GL_TEXTURE_2D, layer->getTexture());

   const float u = layer->getU();
   const float v = layer->getV();

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
      _quad_vertex_buffer = activeDevice->createVertexBuffer(quad_size, true);
   }
   else
   {
      activeDevice->allocateVertexBuffer(_quad_vertex_buffer, quad_size, true);
   }

   void* destination = activeDevice->lockVertexBuffer(_quad_vertex_buffer, quad_size);
   std::memcpy(destination, quad.data(), quad_size);
   activeDevice->unlockVertexBuffer(_quad_vertex_buffer);

   activeDevice->push(Matrix());
   activeDevice->setParameter(activeDevice->getParameterIndex("alpha"), opacity / 255.0f);

   glBindBuffer(GL_ARRAY_BUFFER, _quad_vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, reinterpret_cast<GLvoid*>(sizeof(float) * 3));

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   activeDevice->pop();
}

void MenuPageComboBoxItem::updateTableBounds()
{
   // call base to update maximum height
   MenuPageListItem::updateTableBounds();

   // update layer bounds
   const int top = getCurrentLayer()->getTop();
   const int max_height = getMaxTableHeight();

   getCurrentLayer()->setBottom(top + max_height);
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

   // draw first element layer
   float height = _row_height;
   float width = getLayerFirstElement()->getWidth();
   float x = getLayerFirstElement()->getLeft();
   float y = getLayerFirstElement()->getTop();

   drawQuad(getLayerFirstElement(), x, y, width, height + 1);

   // draw n-1th element layer
   height = _row_height;
   width = getLayerDefaultElement()->getWidth();
   x = getLayerDefaultElement()->getLeft();
   y = getLayerFirstElement()->getTop();

   for (int i = 1; i < element_count - 1; i++)
   {
      y_offset += y_step;
      drawQuad(getLayerDefaultElement(), x, y + y_offset, width, height);
   }

   // draw last element layer
   height = _row_height;
   width = getLayerLastElement()->getWidth();
   x = getLayerLastElement()->getLeft();
   y = getLayerFirstElement()->getTop();

   y_offset += y_step;
   drawQuad(getLayerLastElement(), x, y + y_offset, width, height);

   // draw gradient
   height = element_count * _row_height;
   width = getLayerGradientElement()->getWidth();
   x = getLayerGradientElement()->getLeft();
   y = getLayerFirstElement()->getTop();

   drawQuad(getLayerGradientElement(), x, y, width, height, static_cast<int>(getLayerGradientElement()->getOpacity() * 255.0f));

   MenuPageListItem::draw();
}

void MenuPageComboBoxItem::animate(float time)
{
   MenuPageListItem::animate(time);
}

void MenuPageComboBoxItem::dropDownEnabled(bool /*enabled*/)
{
}

void MenuPageComboBoxItem::addComboBox(const std::string& key, MenuPageComboBoxItem* item)
{
   _map_combo_boxes[key] = item;
}

void MenuPageComboBoxItem::addButton(const std::string& key, MenuPageButtonItem* item)
{
   _map_buttons[key] = item;
}

void MenuPageComboBoxItem::addLabel(const std::string& key, MenuPageLabelItem* item)
{
   _map_labels[key] = item;
}

void MenuPageComboBoxItem::linkComboBoxToButton(const std::string& button_key, const std::string& combo_box_key)
{
   if (_map_buttons.contains(button_key) && _map_combo_boxes.contains(combo_box_key))
   {
      MenuPageButtonItem* button = _map_buttons[button_key];
      MenuPageComboBoxItem* combo_box = _map_combo_boxes[combo_box_key];

      if (!combo_box->getButtonItem())
      {
         combo_box->setButtonItem(button);

         // both items belong to the same MenuPage and share its lifetime
         button->actionSignal.connect([combo_box](const std::string&) { combo_box->setVisible(true); });
      }
   }
}

void MenuPageComboBoxItem::linkComboBoxToLabel(const std::string& label_key, const std::string& combo_box_key)
{
   if (_map_labels.contains(label_key) && _map_combo_boxes.contains(combo_box_key))
   {
      MenuPageLabelItem* label = _map_labels[label_key];
      MenuPageComboBoxItem* combo_box = _map_combo_boxes[combo_box_key];
      combo_box->setLabelItem(label);

      combo_box->valueChangedSignal.connect([label](const std::string& value) { label->setText(value); });
   }
}

void MenuPageComboBoxItem::setButtonItem(MenuPageButtonItem* item)
{
   _button_item = item;
}

MenuPageButtonItem* MenuPageComboBoxItem::getButtonItem() const
{
   return _button_item;
}

MenuPageButtonItem* MenuPageComboBoxItem::getButtonItem(const std::string& name)
{
   const auto iterator = _map_buttons.find(name);
   return iterator != _map_buttons.end() ? iterator->second : nullptr;
}

void MenuPageComboBoxItem::setLabelItem(MenuPageLabelItem* item)
{
   _label_item = item;
}

MenuPageLabelItem* MenuPageComboBoxItem::getLabelItem() const
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
   if (const MenuPageLabelItem* label = getLabelItem())
   {
      return label->getText();
   }

   return {};
}

void MenuPageComboBoxItem::setValue(const std::string& value)
{
   if (MenuPageLabelItem* label = getLabelItem())
   {
      label->setText(value);
   }
}
