#pragma once

#include "gamesignal.h"
#include "menupagelistitem.h"

#include <cstdint>
#include <functional>
#include <optional>

class MenuPageButtonItem;
class MenuPageLabelItem;

/// \brief dropdown list linked to a button (opens it) and a label (shows the value).
///
/// combobox_time_label              // finally displays value
/// combobox_time_button_active      // clicked first
/// combobox_time_button_inactive
/// combobox_time_table_bg_first     // activated after button was clicked
/// combobox_time_table_bg_last
/// combobox_time_table_bg_default
/// combobox_time_table_gradient
class MenuPageComboBoxItem : public MenuPageListItem
{
public:
   MenuPageComboBoxItem();

   void draw() override;

   void animate(float time) override;

   void initialize() override;

   void setFocus(bool) override;

   //! getter for modal flag
   bool isModal() const override;

   //! setter for button item, the combobox opens when it's clicked
   virtual void setButtonItem(MenuPageButtonItem& item);

   //! getter for button item
   std::optional<std::reference_wrapper<MenuPageButtonItem>> getButtonItem() const;

   //! setter for label item, it shows the selected value
   virtual void setLabelItem(MenuPageLabelItem& item);

   //! getter for label item
   std::optional<std::reference_wrapper<MenuPageLabelItem>> getLabelItem() const;

   void mousePressed(int x, int y) override;

   //! getter for value
   std::string getValue() const;

   //! setter for value
   void setValue(const std::string&);

   //! called when button pressed, item selected
   virtual void setVisible(bool visible = true);

   //! dropdown was enabled/disabled
   virtual void dropDownEnabled(bool enabled);

   //! new value is passed to dropdown label on select
   Signal<const std::string&> updateDropDownSignal;

   //! value changed
   Signal<const std::string&> valueChangedSignal;

protected:
   //! draws single quad
   virtual void drawQuad(const PSDLayer& layer, float x, float y, float width, float height, int opacity = 255);

   //! get notified on table bound changes, resize layer
   void updateTableBounds() override;

   //! make combobox visible
   float _visible_animation_time = 0.0f;

   //! make combobox invisible again
   float _invisible_animation_time = 0.0f;

   //! linked button item, on the same page
   std::optional<std::reference_wrapper<MenuPageButtonItem>> _button_item;

   //! linked label item, on the same page
   std::optional<std::reference_wrapper<MenuPageLabelItem>> _label_item;

   //! dynamic vertex buffer reused by drawQuad() every call
   uint32_t _quad_vertex_buffer = 0;
};
