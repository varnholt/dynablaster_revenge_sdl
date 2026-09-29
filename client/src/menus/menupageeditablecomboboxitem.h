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

   //! setter for textedit item
   void setTextEditItem(MenuPageTextEditItem* item);

   //! getter for textedit item
   MenuPageTextEditItem* getTextEditItem() const;

   //! link combobox to related textedit
   static void linkComboBoxToTextEdit(const std::string& text_edit_key, const std::string& combo_box_key);

   //! add textedit to id/ptr-map
   static void addTextEdit(const std::string&, MenuPageTextEditItem*);

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

   //! linked textedit item (non-owning)
   MenuPageTextEditItem* _text_edit_item = nullptr;

   //! registry used while the pages are built, non-owning
   static std::map<std::string, MenuPageTextEditItem*> _map_text_edits;
};
