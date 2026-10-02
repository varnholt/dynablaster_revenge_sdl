#pragma once

#include "charcycling.h"
#include "gamesignal.h"
#include "menus/menupageitem.h"

#include <cstdint>
#include <memory>
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

   struct Element
   {
      MenuPageItem* item = nullptr;
      MenuPageItem* north_item = nullptr;
      MenuPageItem* south_item = nullptr;
      MenuPageItem* east_item = nullptr;
      MenuPageItem* west_item = nullptr;
      Element* north = nullptr;
      Element* south = nullptr;
      Element* east = nullptr;
      Element* west = nullptr;
   };

   explicit MenuControllerGraph(MenuControllerCursorAnimation& animation);
   MenuControllerGraph(const MenuControllerGraph&) = delete;
   MenuControllerGraph& operator=(const MenuControllerGraph&) = delete;
   ~MenuControllerGraph();

   void add(std::unique_ptr<Element> element);

   /// \brief resolves the neighbour items to their elements once all elements are added
   void link();

   MenuPageItem* getDefaultPageItem() const;
   void setDefaultPageItem(MenuPageItem* item);

   /// \brief moves the cursor onto the next item
   void changeFocus(MenuPageItem* current_item, MenuPageItem* next_item);

   void walk(Direction direction);
   void click();

   /// \brief a click at the cursor position is requested
   Signal<int32_t, int32_t> clickSignal;

private:
   void mouseMove(int32_t x, int32_t y);
   MenuPageItem* internalNavigation(Direction direction);
   MenuPageItem* getModalItem() const;
   MenuPageComboBoxItem* getVisibleCombobox() const;
   MenuPageComboBoxItem* getComboBoxForButton(MenuPageItem* button) const;
   MenuPageItem* getActiveItem(MenuPageItem::PageItemType type) const;
   MenuPageSliderItem* getActiveSlider() const;
   MenuPageTextEditItem* getEditingActiveTextEdit() const;
   Element* getFocussedElement() const;
   void autoAdjust();
   void buttonForComboBox(Element* current, MenuPageComboBoxItem* visible_combobox);
   void buttonForSlider(MenuPageSliderItem* activated_slider);
   void buttonForLists(Element* current);
   void buttonForTextEdit(MenuPageTextEditItem* text_edit);
   void updateComboboxFocus(bool visible);
   void disconnectCombobox();

   MenuControllerCursorAnimation& _animation;
   std::vector<std::unique_ptr<Element>> _elements;
   MenuPageItem* _default_page_item = nullptr;
   CharCycling _char_cycling;

   // one-shot: an opened combobox moves the cursor onto its value once it's visible
   MenuPageComboBoxItem* _opened_combobox = nullptr;
   Signal<bool>::Connection _combobox_visible_connection = 0;
};
