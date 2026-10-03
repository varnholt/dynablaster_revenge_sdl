#pragma once

#include "menupagecomboboxitem.h"
#include "menupagetextedit.h"

/// \brief combobox whose value is shown in (and typed into) a linked text edit.
class MenuPageEditableComboBoxItem : public MenuPageComboBoxItem
{
public:
   MenuPageEditableComboBoxItem();

   //! initialize item
   void initialize() override;

   //! setter for visible flag
   void setVisible(bool visible) override;

   //! setter for textedit item, it shows and edits the value
   void setTextEditItem(MenuPageTextEditItem& item);

   //! getter for textedit item
   std::optional<std::reference_wrapper<MenuPageTextEditItem>> getTextEditItem() const;

   //! append an item to the list
   void appendItem(
      const std::string& item,
      const Color& color = Color("#FFFFFF"),
      bool override_alpha = false,
      const Color& outline_color = Color()
   ) override;

protected:
   //! update clipper boundaries
   void updateClipperBounds();

   //! linked textedit item, on the same page
   std::optional<std::reference_wrapper<MenuPageTextEditItem>> _text_edit_item;
};
