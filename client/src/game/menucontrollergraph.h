#pragma once

#include "charcycling.h"
#include "gamesignal.h"
#include "menus/menupageitem.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

class MenuControllerCursorAnimation;
class MenuPageComboBoxItem;
class MenuPageListItem;
class MenuPageSliderItem;
class MenuPageTextEditItem;

/// \brief controller navigation of one menu page: every item knows its north/south/east/west
/// neighbour (data/menus/menu_controller.ini), focus is moved by gliding the cursor onto it.
/// comboboxes, lists, sliders and line edits are navigated internally while they're open.
class MenuControllerGraph
{
public:
   enum class Direction
   {
      North,
      South,
      East,
      West
   };

   using OptionalItem = std::optional<std::reference_wrapper<MenuPageItem>>;

   struct Element
   {
      std::reference_wrapper<MenuPageItem> item;
      OptionalItem north_item;
      OptionalItem south_item;
      OptionalItem east_item;
      OptionalItem west_item;

      // indices into the graph's elements, resolved by link()
      std::optional<size_t> north;
      std::optional<size_t> south;
      std::optional<size_t> east;
      std::optional<size_t> west;
   };

   explicit MenuControllerGraph(MenuControllerCursorAnimation& animation);
   MenuControllerGraph(const MenuControllerGraph&) = delete;
   MenuControllerGraph& operator=(const MenuControllerGraph&) = delete;
   ~MenuControllerGraph();

   void add(Element element);

   /// \brief resolves the neighbour items to their elements once all elements are added
   void link();

   OptionalItem getDefaultPageItem() const;
   void setDefaultPageItem(OptionalItem item);

   /// \brief moves the cursor onto the next item
   void changeFocus(OptionalItem current_item, OptionalItem next_item);

   void walk(Direction direction);
   void click();

   /// \brief a click at the cursor position is requested
   Signal<int32_t, int32_t> clickSignal;

private:
   void mouseMove(int32_t x, int32_t y);
   OptionalItem internalNavigation(Direction direction);
   OptionalItem getModalItem() const;
   std::optional<std::reference_wrapper<MenuPageComboBoxItem>> getVisibleCombobox() const;
   std::optional<std::reference_wrapper<MenuPageComboBoxItem>> getComboBoxForButton(const MenuPageItem& button) const;
   OptionalItem getActiveItem(MenuPageItem::PageItemType type) const;
   std::optional<std::reference_wrapper<MenuPageSliderItem>> getActiveSlider() const;
   std::optional<std::reference_wrapper<MenuPageTextEditItem>> getEditingActiveTextEdit() const;
   std::optional<size_t> findElement(const MenuPageItem& item) const;
   std::optional<size_t> getFocussedElement() const;
   void autoAdjust();
   void buttonForComboBox(std::optional<size_t> current, std::optional<std::reference_wrapper<MenuPageComboBoxItem>> visible_combobox);
   void buttonForSlider(std::optional<std::reference_wrapper<MenuPageSliderItem>> activated_slider);
   void buttonForLists(std::optional<size_t> current);
   void buttonForTextEdit(std::optional<std::reference_wrapper<MenuPageTextEditItem>> text_edit);
   void updateComboboxFocus(bool visible);
   void disconnectCombobox();

   MenuControllerCursorAnimation& _animation;
   std::vector<Element> _elements;
   OptionalItem _default_page_item;
   CharCycling _char_cycling;

   // one-shot: an opened combobox moves the cursor onto its value once it's visible
   std::optional<std::reference_wrapper<MenuPageComboBoxItem>> _opened_combobox;
   Signal<bool>::Connection _combobox_visible_connection = 0;
};
