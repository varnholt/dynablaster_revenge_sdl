#include "menucontrollergraph.h"

#include "menucontrollercursoranimation.h"

#include "menus/menu.h"
#include "menus/menupage.h"
#include "menus/menupagebuttonitem.h"
#include "menus/menupagecheckboxitem.h"
#include "menus/menupagecomboboxitem.h"
#include "menus/menupageeditablecomboboxitem.h"
#include "menus/menupagelistitem.h"
#include "menus/menupagelistitemelement.h"
#include "menus/menupageslideritem.h"
#include "menus/menupagetextedit.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdlib>

namespace
{
// the cursor rests at the golden section of an item, not its center
constexpr float golden_ratio = 0.6180339887f;

MenuPage* currentPage()
{
   return Menu::getInstance()->getCurrentPage();
}
}  // namespace

MenuControllerGraph::MenuControllerGraph(MenuControllerCursorAnimation& animation) : _animation(animation)
{
}

MenuControllerGraph::~MenuControllerGraph()
{
   disconnectCombobox();
}

void MenuControllerGraph::add(std::unique_ptr<Element> element)
{
   _elements.push_back(std::move(element));
}

void MenuControllerGraph::link()
{
   const auto find = [this](MenuPageItem* item) -> Element*
   {
      if (!item)
      {
         return nullptr;
      }
      const auto it = std::ranges::find(_elements, item, &Element::item);
      return it != _elements.end() ? it->get() : nullptr;
   };

   for (const auto& element : _elements)
   {
      element->north = find(element->north_item);
      element->south = find(element->south_item);
      element->east = find(element->east_item);
      element->west = find(element->west_item);
   }
}

MenuPageItem* MenuControllerGraph::getDefaultPageItem() const
{
   return _default_page_item;
}

void MenuControllerGraph::setDefaultPageItem(MenuPageItem* item)
{
   _default_page_item = item;
}

void MenuControllerGraph::mouseMove(int32_t x, int32_t y)
{
   // a negative coordinate keeps the cursor's current one
   _animation.add(x < 0 ? _animation.getX() : x, y < 0 ? _animation.getY() : y);
}

void MenuControllerGraph::changeFocus(MenuPageItem* current_item, MenuPageItem* next_item)
{
   if (!next_item)
   {
      return;
   }

   PSDLayer* layer = nullptr;
   int32_t top = 0;
   int32_t bottom = 0;
   int32_t left = 0;
   int32_t right = 0;

   switch (next_item->getPageItemType())
   {
      case MenuPageItem::PageItemTypeList:
      case MenuPageItem::PageItemTypeSlider:
      case MenuPageItem::PageItemTypeTextedit:
      {
         // active ones are navigated internally
         if (next_item->isActive())
         {
            return;
         }
         layer = next_item->getActiveLayer();
         break;
      }
      case MenuPageItem::PageItemTypeCheckbox:
      {
         layer = static_cast<MenuPageCheckBoxItem*>(next_item)->getCheckedLayer();
         break;
      }
      case MenuPageItem::PageItemTypeListElement:
      {
         auto* element = static_cast<MenuPageListItemElement*>(next_item);
         MenuPageItem* parent = element->getParent();

         // an editable combobox' button spans the area behind its line edit, the elements are
         // placed relative to the line edit then
         int32_t base_top = 0;
         int32_t base_left = 0;
         if (parent && parent->getPageItemType() == MenuPageItem::PageItemTypeEditableCombobox)
         {
            auto* combo = static_cast<MenuPageEditableComboBoxItem*>(parent);
            base_top = combo->getTextEditItem()->getActiveLayer()->getTop();
            base_left = combo->getTextEditItem()->getActiveLayer()->getLeft();
         }
         else if (current_item && current_item->getActiveLayer())
         {
            base_top = current_item->getActiveLayer()->getTop();
            base_left = current_item->getActiveLayer()->getLeft();
         }

         top = base_top + static_cast<int32_t>(element->getY());
         bottom = top + element->getHeight();
         left = base_left + static_cast<int32_t>(element->getX());
         right = left + element->getWidth();
         break;
      }
      default:
      {
         layer = next_item->getActiveLayer();
         break;
      }
   }

   if (layer && layer->getLayer())
   {
      top = layer->getTop();
      bottom = layer->getBottom();
      left = layer->getLeft();
      right = layer->getRight();
   }

   const auto x = left + static_cast<int32_t>(std::abs(left - right) * golden_ratio);
   const auto y = top + static_cast<int32_t>(std::abs(top - bottom) * golden_ratio);
   mouseMove(x, y);
}

MenuPageItem* MenuControllerGraph::internalNavigation(Direction direction)
{
   MenuPage* page = currentPage();
   if (!page)
   {
      return nullptr;
   }

   for (const auto& page_item : page->getPageItems())
   {
      MenuPageItem* item = page_item.get();
      const auto type = item->getPageItemType();

      // an opened combobox: walk its elements
      if (type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox)
      {
         if (item->isVisible())
         {
            auto* combo = static_cast<MenuPageComboBoxItem*>(item);
            int32_t next = combo->getFocussedElement();
            if (direction == Direction::North)
            {
               next = std::max(0, next - 1);
            }
            else if (direction == Direction::South)
            {
               next = std::min(combo->getElementCount() - 1, next + 1);
            }
            return combo->getElementAt(next);
         }
      }

      // an active list: scroll through its rows
      else if (type == MenuPageItem::PageItemTypeList)
      {
         if (item->isActive())
         {
            auto* list = static_cast<MenuPageListItem*>(item);
            int32_t focussed = list->getFocussedElement();
            switch (direction)
            {
               case Direction::North:
                  focussed--;
                  break;
               case Direction::South:
                  focussed++;
                  break;
               case Direction::East:
                  focussed = list->getElementCount() - 1;
                  break;
               case Direction::West:
                  focussed = 0;
                  break;
            }

            if (focussed > -1 && focussed < list->getElementCount())
            {
               constexpr int32_t offset_y = 20;
               mouseMove(-1, list->scrollSmoothToIndex(focussed) + offset_y);
            }
            return item;
         }
      }

      // a grabbed slider: east/west in large steps, north/south fine-tuned
      else if (type == MenuPageItem::PageItemTypeSlider)
      {
         if (item->isActive())
         {
            auto* slider = static_cast<MenuPageSliderItem*>(item);
            constexpr int32_t step = 40;
            constexpr int32_t fine_step = 10;
            int32_t x = _animation.getX();
            switch (direction)
            {
               case Direction::East:
                  x = std::min(x + step, slider->getMaximum());
                  break;
               case Direction::West:
                  x = std::max(x - step, slider->getMinimum());
                  break;
               case Direction::North:
                  x = std::min(x + fine_step, slider->getMaximum());
                  break;
               case Direction::South:
                  x = std::max(x - fine_step, slider->getMinimum());
                  break;
            }
            mouseMove(x, -1);
            return item;
         }
      }

      // a line edit in edit mode: north/south cycle the character, east/west move the cursor
      else if (type == MenuPageItem::PageItemTypeTextedit)
      {
         auto* text_edit = static_cast<MenuPageTextEditItem*>(item);
         if (text_edit->isEditingActive())
         {
            const std::string text = text_edit->getText();
            const auto length = static_cast<int32_t>(text.size());
            const int32_t cursor_position = text_edit->getCursorPosition();
            const char current_char = cursor_position < length ? text[static_cast<size_t>(cursor_position)] : ' ';
            bool update_text_edit = false;

            switch (direction)
            {
               case Direction::East:
                  if (length < text_edit->getMaxLength() - 1)
                  {
                     text_edit->keyPressed(SDLK_RIGHT, "");
                  }
                  break;
               case Direction::West:
                  text_edit->keyPressed(SDLK_RIGHT, "");
                  text_edit->keyPressed(SDLK_BACKSPACE, "");
                  break;
               case Direction::North:
                  if (length < text_edit->getMaxLength())
                  {
                     _char_cycling.setChar(current_char);
                     _char_cycling.up();
                     update_text_edit = true;
                  }
                  break;
               case Direction::South:
                  if (length < text_edit->getMaxLength())
                  {
                     _char_cycling.setChar(current_char);
                     _char_cycling.down();
                     update_text_edit = true;
                  }
                  break;
            }

            if (update_text_edit)
            {
               text_edit->setText(_char_cycling.modify(text, text_edit->getCursorPosition()));
               text_edit->setCursorPosition(cursor_position);
            }
            return item;
         }
      }
   }

   return nullptr;
}

void MenuControllerGraph::updateComboboxFocus(bool visible)
{
   MenuPageComboBoxItem* combobox = _opened_combobox;
   disconnectCombobox();

   if (!combobox || !visible)
   {
      return;
   }

   // once the combobox shows its value, move the cursor onto it
   Element* current = getFocussedElement();
   if (!current)
   {
      return;
   }

   int32_t index = 0;
   for (int32_t i = 0; i < combobox->getElementCount(); ++i)
   {
      if (combobox->getValue() == combobox->getElementAt(i)->getText())
      {
         index = i;
      }
   }

   if (MenuPageItem* focussed_element = combobox->getElementAt(index))
   {
      changeFocus(current->item, focussed_element);
   }
}

void MenuControllerGraph::disconnectCombobox()
{
   if (_opened_combobox)
   {
      _opened_combobox->visibleSignal.disconnect(_combobox_visible_connection);
      _opened_combobox = nullptr;
   }
}

MenuPageItem* MenuControllerGraph::getModalItem() const
{
   MenuPage* page = currentPage();
   if (!page)
   {
      return nullptr;
   }

   for (const auto& page_item : page->getPageItems())
   {
      MenuPageItem* item = page_item.get();
      const auto type = item->getPageItemType();

      // an opened combobox, its button is the navigation's parent element
      if (type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox)
      {
         if (item->isVisible())
         {
            return static_cast<MenuPageComboBoxItem*>(item)->getButtonItem();
         }
      }

      // a grabbed slider
      else if (type == MenuPageItem::PageItemTypeSlider)
      {
         if (item->isActive())
         {
            return item;
         }
      }
   }

   return nullptr;
}

MenuPageComboBoxItem* MenuControllerGraph::getVisibleCombobox() const
{
   MenuPage* page = currentPage();
   if (!page)
   {
      return nullptr;
   }

   for (const auto& page_item : page->getPageItems())
   {
      const auto type = page_item->getPageItemType();
      if ((type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox) && page_item->isVisible())
      {
         return static_cast<MenuPageComboBoxItem*>(page_item.get());
      }
   }

   return nullptr;
}

MenuPageComboBoxItem* MenuControllerGraph::getComboBoxForButton(MenuPageItem* button) const
{
   MenuPage* page = currentPage();
   if (!page || !button)
   {
      return nullptr;
   }

   MenuPageComboBoxItem* combobox = nullptr;
   for (const auto& page_item : page->getPageItems())
   {
      const auto type = page_item->getPageItemType();
      if (type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox)
      {
         auto* candidate = static_cast<MenuPageComboBoxItem*>(page_item.get());
         if (candidate->getButtonItem() == button)
         {
            combobox = candidate;
         }
      }
   }

   return combobox;
}

MenuPageItem* MenuControllerGraph::getActiveItem(MenuPageItem::PageItemType type) const
{
   MenuPage* page = currentPage();
   if (!page)
   {
      return nullptr;
   }

   for (const auto& page_item : page->getPageItems())
   {
      if (page_item->getPageItemType() == type && page_item->isActive())
      {
         return page_item.get();
      }
   }

   return nullptr;
}

MenuPageSliderItem* MenuControllerGraph::getActiveSlider() const
{
   return static_cast<MenuPageSliderItem*>(getActiveItem(MenuPageItem::PageItemTypeSlider));
}

MenuPageTextEditItem* MenuControllerGraph::getEditingActiveTextEdit() const
{
   MenuPage* page = currentPage();
   if (!page)
   {
      return nullptr;
   }

   for (const auto& page_item : page->getPageItems())
   {
      if (page_item->getPageItemType() == MenuPageItem::PageItemTypeTextedit)
      {
         auto* text_edit = static_cast<MenuPageTextEditItem*>(page_item.get());
         if (text_edit->isEditingActive())
         {
            return text_edit;
         }
      }
   }

   return nullptr;
}

void MenuControllerGraph::autoAdjust()
{
   // nothing focussed: pick the element closest to the cursor (manhattan distance)
   const int32_t mouse_x = _animation.getX();
   const int32_t mouse_y = _animation.getY();
   Element* closest = nullptr;
   int32_t closest_distance = 0;

   for (const auto& element : _elements)
   {
      PSD::Layer* layer = element->item ? element->item->getCurrentLayer() : nullptr;
      if (!layer)
      {
         continue;
      }

      const int32_t x = layer->getLeft() + layer->getWidth() / 2;
      const int32_t y = layer->getTop() + layer->getHeight() / 2;
      const int32_t distance = std::abs(x - mouse_x) + std::abs(y - mouse_y);
      if (!closest || distance < closest_distance)
      {
         closest = element.get();
         closest_distance = distance;
      }
   }

   if (closest)
   {
      changeFocus(nullptr, closest->item);
   }
}

void MenuControllerGraph::walk(Direction direction)
{
   Element* current = getFocussedElement();
   if (!current)
   {
      autoAdjust();
      return;
   }

   Element* next = nullptr;
   switch (direction)
   {
      case Direction::North:
         next = current->north;
         break;
      case Direction::South:
         next = current->south;
         break;
      case Direction::East:
         next = current->east;
         break;
      case Direction::West:
         next = current->west;
         break;
   }

   MenuPageItem* internal = internalNavigation(direction);
   changeFocus(current->item, internal ? internal : (next ? next->item : nullptr));
}

void MenuControllerGraph::buttonForComboBox(Element* current, MenuPageComboBoxItem* visible_combobox)
{
   // a combobox that was just opened: move the cursor onto its value once it's shown
   if (current)
   {
      if (MenuPageComboBoxItem* opened = getComboBoxForButton(current->item))
      {
         disconnectCombobox();
         _opened_combobox = opened;
         _combobox_visible_connection = opened->visibleSignal.connect([this](bool visible) { updateComboboxFocus(visible); });
      }
   }

   // a combobox that was just closed: back to its button
   if (visible_combobox && current)
   {
      changeFocus(current->item, visible_combobox->getButtonItem());
   }
}

void MenuControllerGraph::buttonForSlider(MenuPageSliderItem* activated_slider)
{
   if (activated_slider)
   {
      activated_slider->mouseReleased();
   }
}

void MenuControllerGraph::buttonForLists(Element* current)
{
   if (!current || current->item->getPageItemType() != MenuPageItem::PageItemTypeList)
   {
      return;
   }

   // toggles between scrolling through the rows and selecting the focussed one
   auto* list = static_cast<MenuPageListItem*>(current->item);
   list->setActive(!list->isActive());
   if (list->isActive())
   {
      list->scrollSmoothToIndex(list->getActiveElement());
   }
   else
   {
      list->setActiveElement(list->getFocussedElement());
   }
}

void MenuControllerGraph::buttonForTextEdit(MenuPageTextEditItem* text_edit)
{
   // it was active before, so it only needs to be deactivated
   if (text_edit && text_edit->isEditingActive())
   {
      text_edit->deactivated();
   }
}

void MenuControllerGraph::click()
{
   Element* current = getFocussedElement();

   // modal items
   MenuPageComboBoxItem* visible_combobox = getVisibleCombobox();
   MenuPageSliderItem* activated_slider = getActiveSlider();
   MenuPageTextEditItem* active_text_edit = getEditingActiveTextEdit();

   buttonForLists(current);

   // mostly every other item
   if (current && !activated_slider && !active_text_edit)
   {
      clickSignal(_animation.getX(), _animation.getY());
   }

   buttonForTextEdit(active_text_edit);
   buttonForSlider(activated_slider);
   buttonForComboBox(current, visible_combobox);
}

MenuControllerGraph::Element* MenuControllerGraph::getFocussedElement() const
{
   MenuPage* page = currentPage();
   if (!page)
   {
      return nullptr;
   }

   // an opened combobox or grabbed slider wins over whatever is under the cursor
   std::vector<MenuPageItem*> focussed_items;
   if (MenuPageItem* modal_item = getModalItem())
   {
      focussed_items.push_back(modal_item);
   }
   else
   {
      focussed_items = page->getItemsAt(_animation.getX(), _animation.getY());
   }

   Element* found = nullptr;
   for (MenuPageItem* item : focussed_items)
   {
      const auto it = std::ranges::find(_elements, item, &Element::item);
      if (it != _elements.end())
      {
         found = it->get();
      }
   }

   return found;
}
