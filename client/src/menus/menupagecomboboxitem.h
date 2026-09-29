#pragma once

#include "menupagelistitem.h"
#include "signal.h"

#include <cstdint>
#include <map>

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

   //! add combobox to id/ptr-map
   static void addComboBox(const std::string&, MenuPageComboBoxItem*);

   //! add button to id/ptr-map
   static void addButton(const std::string&, MenuPageButtonItem*);

   //! add label to id/ptr-map
   static void addLabel(const std::string&, MenuPageLabelItem*);

   //! link combobox to related button
   static void linkComboBoxToButton(const std::string& button_key, const std::string& combo_box_key);

   //! link combobox to related label
   static void linkComboBoxToLabel(const std::string& label_key, const std::string& combo_box_key);

   void initialize() override;

   void setFocus(bool) override;

   //! getter for modal flag
   bool isModal() const override;

   //! setter for button item
   virtual void setButtonItem(MenuPageButtonItem* item);

   //! getter for button item
   MenuPageButtonItem* getButtonItem() const;

   //! getter for button item
   static MenuPageButtonItem* getButtonItem(const std::string& name);

   //! setter for label item
   virtual void setLabelItem(MenuPageLabelItem* item);

   //! getter for label item
   MenuPageLabelItem* getLabelItem() const;

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
   virtual void drawQuad(PSDLayer* layer, float x, float y, float width, float height, int opacity = 255);

   //! get notified on table bound changes, resize layer
   void updateTableBounds() override;

   //! make combobox visible
   float _visible_animation_time = 0.0f;

   //! make combobox invisible again
   float _invisible_animation_time = 0.0f;

   //! linked button item (non-owning)
   MenuPageButtonItem* _button_item = nullptr;

   //! linked label item (non-owning)
   MenuPageLabelItem* _label_item = nullptr;

   //! dynamic vertex buffer reused by drawQuad() every call
   uint32_t _quad_vertex_buffer = 0;

   // registries used while the pages are built, non-owning
   static std::map<std::string, MenuPageComboBoxItem*> _map_combo_boxes;
   static std::map<std::string, MenuPageButtonItem*> _map_buttons;
   static std::map<std::string, MenuPageLabelItem*> _map_labels;
};
