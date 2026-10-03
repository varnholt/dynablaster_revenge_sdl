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

std::optional<std::reference_wrapper<MenuPage>> currentPage()
{
   return Menu::getInstance().getCurrentPage();
}

template <typename T>
MenuControllerGraph::OptionalItem asItem(const std::optional<std::reference_wrapper<T>>& item)
{
   if (item)
   {
      return item->get();
   }
   return std::nullopt;
}
}  // namespace

MenuControllerGraph::MenuControllerGraph(MenuControllerCursorAnimation& animation) : _animation(animation)
{
}

MenuControllerGraph::~MenuControllerGraph()
{
   disconnectCombobox();
}

void MenuControllerGraph::add(Element element)
{
   _elements.push_back(std::move(element));
}

std::optional<size_t> MenuControllerGraph::findElement(const MenuPageItem& item) const
{
   const auto it = std::ranges::find_if(_elements, [&item](const Element& element) { return &element.item.get() == &item; });
   if (it != _elements.end())
   {
      return static_cast<size_t>(it - _elements.begin());
   }
   return std::nullopt;
}

void MenuControllerGraph::link()
{
   const auto find = [this](const OptionalItem& item) -> std::optional<size_t>
   {
      if (!item)
      {
         return std::nullopt;
      }
      return findElement(*item);
   };

   for (auto& element : _elements)
   {
      element.north = find(element.north_item);
      element.south = find(element.south_item);
      element.east = find(element.east_item);
      element.west = find(element.west_item);
   }
}

MenuControllerGraph::OptionalItem MenuControllerGraph::getDefaultPageItem() const
{
   return _default_page_item;
}

void MenuControllerGraph::setDefaultPageItem(OptionalItem item)
{
   _default_page_item = item;
}

void MenuControllerGraph::mouseMove(int32_t x, int32_t y)
{
   // a negative coordinate keeps the cursor's current one
   _animation.add(x < 0 ? _animation.getX() : x, y < 0 ? _animation.getY() : y);
}

void MenuControllerGraph::changeFocus(OptionalItem current_item, OptionalItem next_item)
{
   if (!next_item)
   {
      return;
   }

   MenuPageItem& next = *next_item;

   std::optional<std::reference_wrapper<PSDLayer>> layer;
   int32_t top = 0;
   int32_t bottom = 0;
   int32_t left = 0;
   int32_t right = 0;

   switch (next.getPageItemType())
   {
      case MenuPageItem::PageItemTypeList:
      case MenuPageItem::PageItemTypeSlider:
      case MenuPageItem::PageItemTypeTextedit:
      {
         // active ones are navigated internally
         if (next.isActive())
         {
            return;
         }
         layer = next.getActiveLayer();
         break;
      }
      case MenuPageItem::PageItemTypeCheckbox:
      {
         layer = static_cast<MenuPageCheckBoxItem&>(next).getCheckedLayer();
         break;
      }
      case MenuPageItem::PageItemTypeListElement:
      {
         const auto& element = static_cast<MenuPageListItemElement&>(next);
         const MenuPageListItem& parent = element.getParent();

         // an editable combobox' button spans the area behind its line edit, the elements are
         // placed relative to the line edit then
         int32_t base_top = 0;
         int32_t base_left = 0;
         if (parent.getPageItemType() == MenuPageItem::PageItemTypeEditableCombobox)
         {
            const auto& combo = static_cast<const MenuPageEditableComboBoxItem&>(parent);
            const PSDLayer& text_edit_layer = combo.getTextEditItem()->get().getActiveLayer()->get();
            base_top = text_edit_layer.getTop();
            base_left = text_edit_layer.getLeft();
         }
         else if (current_item && current_item->get().getActiveLayer())
         {
            const PSDLayer& current_layer = current_item->get().getActiveLayer()->get();
            base_top = current_layer.getTop();
            base_left = current_layer.getLeft();
         }

         top = base_top + static_cast<int32_t>(element.getY());
         bottom = top + element.getHeight();
         left = base_left + static_cast<int32_t>(element.getX());
         right = left + element.getWidth();
         break;
      }
      default:
      {
         layer = next.getActiveLayer();
         break;
      }
   }

   if (layer)
   {
      top = layer->get().getTop();
      bottom = layer->get().getBottom();
      left = layer->get().getLeft();
      right = layer->get().getRight();
   }

   const auto x = left + static_cast<int32_t>(std::abs(left - right) * golden_ratio);
   const auto y = top + static_cast<int32_t>(std::abs(top - bottom) * golden_ratio);
   mouseMove(x, y);
}

MenuControllerGraph::OptionalItem MenuControllerGraph::internalNavigation(Direction direction)
{
   const auto page = currentPage();
   if (!page)
   {
      return std::nullopt;
   }

   for (const auto& page_item : page->get().getPageItems())
   {
      MenuPageItem& item = *page_item;
      const auto type = item.getPageItemType();

      // an opened combobox: walk its elements
      if (type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox)
      {
         if (item.isVisible())
         {
            auto& combo = static_cast<MenuPageComboBoxItem&>(item);
            int32_t next = combo.getFocussedElement();
            if (direction == Direction::North)
            {
               next = std::max(0, next - 1);
            }
            else if (direction == Direction::South)
            {
               next = std::min(combo.getElementCount() - 1, next + 1);
            }
            return asItem(combo.getElementAt(next));
         }
      }

      // an active list: scroll through its rows
      else if (type == MenuPageItem::PageItemTypeList)
      {
         if (item.isActive())
         {
            auto& list = static_cast<MenuPageListItem&>(item);
            int32_t focussed = list.getFocussedElement();
            switch (direction)
            {
               case Direction::North:
                  focussed--;
                  break;
               case Direction::South:
                  focussed++;
                  break;
               case Direction::East:
                  focussed = list.getElementCount() - 1;
                  break;
               case Direction::West:
                  focussed = 0;
                  break;
            }

            if (focussed > -1 && focussed < list.getElementCount())
            {
               constexpr int32_t offset_y = 20;
               mouseMove(-1, list.scrollSmoothToIndex(focussed) + offset_y);
            }
            return item;
         }
      }

      // a grabbed slider: east/west in large steps, north/south fine-tuned
      else if (type == MenuPageItem::PageItemTypeSlider)
      {
         if (item.isActive())
         {
            const auto& slider = static_cast<const MenuPageSliderItem&>(item);
            constexpr int32_t step = 40;
            constexpr int32_t fine_step = 10;
            int32_t x = _animation.getX();
            switch (direction)
            {
               case Direction::East:
                  x = std::min(x + step, slider.getMaximum());
                  break;
               case Direction::West:
                  x = std::max(x - step, slider.getMinimum());
                  break;
               case Direction::North:
                  x = std::min(x + fine_step, slider.getMaximum());
                  break;
               case Direction::South:
                  x = std::max(x - fine_step, slider.getMinimum());
                  break;
            }
            mouseMove(x, -1);
            return item;
         }
      }

      // a line edit in edit mode: north/south cycle the character, east/west move the cursor
      else if (type == MenuPageItem::PageItemTypeTextedit)
      {
         auto& text_edit = static_cast<MenuPageTextEditItem&>(item);
         if (text_edit.isEditingActive())
         {
            const std::string text = text_edit.getText();
            const auto length = static_cast<int32_t>(text.size());
            const int32_t cursor_position = text_edit.getCursorPosition();
            const char current_char = cursor_position < length ? text[static_cast<size_t>(cursor_position)] : ' ';
            bool update_text_edit = false;

            switch (direction)
            {
               case Direction::East:
                  if (length < text_edit.getMaxLength() - 1)
                  {
                     text_edit.keyPressed(SDLK_RIGHT, "");
                  }
                  break;
               case Direction::West:
                  text_edit.keyPressed(SDLK_RIGHT, "");
                  text_edit.keyPressed(SDLK_BACKSPACE, "");
                  break;
               case Direction::North:
                  if (length < text_edit.getMaxLength())
                  {
                     _char_cycling.setChar(current_char);
                     _char_cycling.up();
                     update_text_edit = true;
                  }
                  break;
               case Direction::South:
                  if (length < text_edit.getMaxLength())
                  {
                     _char_cycling.setChar(current_char);
                     _char_cycling.down();
                     update_text_edit = true;
                  }
                  break;
            }

            if (update_text_edit)
            {
               text_edit.setText(_char_cycling.modify(text, text_edit.getCursorPosition()));
               text_edit.setCursorPosition(cursor_position);
            }
            return item;
         }
      }
   }

   return std::nullopt;
}

void MenuControllerGraph::updateComboboxFocus(bool visible)
{
   const auto opened = _opened_combobox;
   disconnectCombobox();

   if (!opened || !visible)
   {
      return;
   }

   // once the combobox shows its value, move the cursor onto it
   const auto current = getFocussedElement();
   if (!current)
   {
      return;
   }

   MenuPageComboBoxItem& combobox = *opened;

   int32_t index = 0;
   for (int32_t i = 0; i < combobox.getElementCount(); ++i)
   {
      if (combobox.getValue() == combobox.getElementAt(i)->get().getText())
      {
         index = i;
      }
   }

   if (const auto focussed_element = combobox.getElementAt(index))
   {
      changeFocus(_elements[*current].item, focussed_element->get());
   }
}

void MenuControllerGraph::disconnectCombobox()
{
   if (_opened_combobox)
   {
      _opened_combobox->get().visibleSignal.disconnect(_combobox_visible_connection);
      _opened_combobox.reset();
   }
}

MenuControllerGraph::OptionalItem MenuControllerGraph::getModalItem() const
{
   const auto page = currentPage();
   if (!page)
   {
      return std::nullopt;
   }

   for (const auto& page_item : page->get().getPageItems())
   {
      MenuPageItem& item = *page_item;
      const auto type = item.getPageItemType();

      // an opened combobox, its button is the navigation's parent element
      if (type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox)
      {
         if (item.isVisible())
         {
            return asItem(static_cast<MenuPageComboBoxItem&>(item).getButtonItem());
         }
      }

      // a grabbed slider
      else if (type == MenuPageItem::PageItemTypeSlider)
      {
         if (item.isActive())
         {
            return item;
         }
      }
   }

   return std::nullopt;
}

std::optional<std::reference_wrapper<MenuPageComboBoxItem>> MenuControllerGraph::getVisibleCombobox() const
{
   const auto page = currentPage();
   if (!page)
   {
      return std::nullopt;
   }

   for (const auto& page_item : page->get().getPageItems())
   {
      const auto type = page_item->getPageItemType();
      if ((type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox) && page_item->isVisible())
      {
         return static_cast<MenuPageComboBoxItem&>(*page_item);
      }
   }

   return std::nullopt;
}

std::optional<std::reference_wrapper<MenuPageComboBoxItem>> MenuControllerGraph::getComboBoxForButton(const MenuPageItem& button) const
{
   const auto page = currentPage();
   if (!page)
   {
      return std::nullopt;
   }

   std::optional<std::reference_wrapper<MenuPageComboBoxItem>> combobox;
   for (const auto& page_item : page->get().getPageItems())
   {
      const auto type = page_item->getPageItemType();
      if (type == MenuPageItem::PageItemTypeCombobox || type == MenuPageItem::PageItemTypeEditableCombobox)
      {
         auto& candidate = static_cast<MenuPageComboBoxItem&>(*page_item);
         const auto candidate_button = candidate.getButtonItem();
         if (candidate_button && &candidate_button->get() == &button)
         {
            combobox = candidate;
         }
      }
   }

   return combobox;
}

MenuControllerGraph::OptionalItem MenuControllerGraph::getActiveItem(MenuPageItem::PageItemType type) const
{
   const auto page = currentPage();
   if (!page)
   {
      return std::nullopt;
   }

   for (const auto& page_item : page->get().getPageItems())
   {
      if (page_item->getPageItemType() == type && page_item->isActive())
      {
         return *page_item;
      }
   }

   return std::nullopt;
}

std::optional<std::reference_wrapper<MenuPageSliderItem>> MenuControllerGraph::getActiveSlider() const
{
   if (const auto item = getActiveItem(MenuPageItem::PageItemTypeSlider))
   {
      return static_cast<MenuPageSliderItem&>(item->get());
   }
   return std::nullopt;
}

std::optional<std::reference_wrapper<MenuPageTextEditItem>> MenuControllerGraph::getEditingActiveTextEdit() const
{
   const auto page = currentPage();
   if (!page)
   {
      return std::nullopt;
   }

   for (const auto& page_item : page->get().getPageItems())
   {
      if (page_item->getPageItemType() == MenuPageItem::PageItemTypeTextedit)
      {
         auto& text_edit = static_cast<MenuPageTextEditItem&>(*page_item);
         if (text_edit.isEditingActive())
         {
            return text_edit;
         }
      }
   }

   return std::nullopt;
}

void MenuControllerGraph::autoAdjust()
{
   // nothing focussed: pick the element closest to the cursor (manhattan distance)
   const int32_t mouse_x = _animation.getX();
   const int32_t mouse_y = _animation.getY();
   std::optional<size_t> closest;
   int32_t closest_distance = 0;

   for (size_t index = 0; index < _elements.size(); index++)
   {
      const auto layer = _elements[index].item.get().getCurrentLayer();
      if (!layer)
      {
         continue;
      }

      const PSD::Layer& bounds = *layer;
      const int32_t x = bounds.getLeft() + bounds.getWidth() / 2;
      const int32_t y = bounds.getTop() + bounds.getHeight() / 2;
      const int32_t distance = std::abs(x - mouse_x) + std::abs(y - mouse_y);
      if (!closest || distance < closest_distance)
      {
         closest = index;
         closest_distance = distance;
      }
   }

   if (closest)
   {
      changeFocus(std::nullopt, _elements[*closest].item);
   }
}

void MenuControllerGraph::walk(Direction direction)
{
   const auto current = getFocussedElement();
   if (!current)
   {
      autoAdjust();
      return;
   }

   const Element& element = _elements[*current];

   std::optional<size_t> next;
   switch (direction)
   {
      case Direction::North:
         next = element.north;
         break;
      case Direction::South:
         next = element.south;
         break;
      case Direction::East:
         next = element.east;
         break;
      case Direction::West:
         next = element.west;
         break;
   }

   const OptionalItem internal = internalNavigation(direction);
   changeFocus(element.item, internal ? internal : (next ? OptionalItem{_elements[*next].item} : std::nullopt));
}

void MenuControllerGraph::buttonForComboBox(
   std::optional<size_t> current,
   std::optional<std::reference_wrapper<MenuPageComboBoxItem>> visible_combobox
)
{
   // a combobox that was just opened: move the cursor onto its value once it's shown
   if (current)
   {
      if (const auto opened = getComboBoxForButton(_elements[*current].item))
      {
         disconnectCombobox();
         _opened_combobox = opened;
         _combobox_visible_connection = opened->get().visibleSignal.connect([this](bool visible) { updateComboboxFocus(visible); });
      }
   }

   // a combobox that was just closed: back to its button
   if (visible_combobox && current)
   {
      changeFocus(_elements[*current].item, asItem(visible_combobox->get().getButtonItem()));
   }
}

void MenuControllerGraph::buttonForSlider(std::optional<std::reference_wrapper<MenuPageSliderItem>> activated_slider)
{
   if (activated_slider)
   {
      activated_slider->get().mouseReleased();
   }
}

void MenuControllerGraph::buttonForLists(std::optional<size_t> current)
{
   if (!current || _elements[*current].item.get().getPageItemType() != MenuPageItem::PageItemTypeList)
   {
      return;
   }

   // toggles between scrolling through the rows and selecting the focussed one
   auto& list = static_cast<MenuPageListItem&>(_elements[*current].item.get());
   list.setActive(!list.isActive());
   if (list.isActive())
   {
      list.scrollSmoothToIndex(list.getActiveElement());
   }
   else
   {
      list.setActiveElement(list.getFocussedElement());
   }
}

void MenuControllerGraph::buttonForTextEdit(std::optional<std::reference_wrapper<MenuPageTextEditItem>> text_edit)
{
   // it was active before, so it only needs to be deactivated
   if (text_edit && text_edit->get().isEditingActive())
   {
      text_edit->get().deactivated();
   }
}

void MenuControllerGraph::click()
{
   const auto current = getFocussedElement();

   // modal items
   const auto visible_combobox = getVisibleCombobox();
   const auto activated_slider = getActiveSlider();
   const auto active_text_edit = getEditingActiveTextEdit();

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

std::optional<size_t> MenuControllerGraph::getFocussedElement() const
{
   const auto page = currentPage();
   if (!page)
   {
      return std::nullopt;
   }

   // an opened combobox or grabbed slider wins over whatever is under the cursor
   std::vector<std::reference_wrapper<MenuPageItem>> focussed_items;
   if (const auto modal_item = getModalItem())
   {
      focussed_items.push_back(*modal_item);
   }
   else
   {
      focussed_items = page->get().getItemsAt(_animation.getX(), _animation.getY());
   }

   std::optional<size_t> found;
   for (const MenuPageItem& item : focussed_items)
   {
      if (const auto index = findElement(item))
      {
         found = index;
      }
   }

   return found;
}
