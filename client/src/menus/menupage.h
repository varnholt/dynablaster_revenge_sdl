#pragma once

#include "gamesignal.h"
#include "settings.h"

#include "image/psd.h"
#include "menupageitem.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

class MenuPageAnimation;
class PSDLayer;

/// \brief one menu page built from a PSD file: every layer becomes a PSDLayer, and layers are
/// grouped into page items by their name prefix (combobox, editablecombobox, checkbox, button,
/// label, lineedit, background, pixmap, table_*, slider_, image_scroll, default item).
class MenuPage : public PSD
{
public:
   MenuPage();
   ~MenuPage();

   MenuPage(const MenuPage&) = delete;
   MenuPage& operator=(const MenuPage&) = delete;

   void setTitle(const std::string&);

   void setFilename(const std::string&);

   std::string getFilename() const;

   void initialize();

   const std::vector<std::unique_ptr<MenuPageItem>>& getPageItems() const;

   void setActive(bool);

   bool isActive();

   //! the animation is owned by the MenuDrawable
   void setAnimation(MenuPageAnimation&);

   std::optional<std::reference_wrapper<MenuPageAnimation>> getAnimation() const;

   //! the page item of that name if it is a T
   template <typename T = MenuPageItem>
   std::optional<std::reference_wrapper<T>> getPageItem(const std::string& layer_name) const;

   //! getter for the active item
   std::optional<std::reference_wrapper<MenuPageItem>> getActiveItem() const;

   //! setter for the active item
   void setActiveItem(std::optional<std::reference_wrapper<MenuPageItem>> value);

   //! getter for pageitem at given position
   std::vector<std::reference_wrapper<MenuPageItem>> getItemsAt(int x, int y) const;

   //! getter for the focussed item
   std::optional<std::reference_wrapper<MenuPageItem>> getFocussedItem() const;

   //! a repeated layer group (see repeatGroups()) has this many instances, 0 if it isn't one
   int32_t getGroupInstanceCount(const std::string& group) const;

   //! moves one instance of a repeated group horizontally, relative to where the PSD has it
   void setGroupInstanceOffset(const std::string& group, int32_t index, int32_t offset);

   //! name of a layer's copy in a repeated group's instance (1-based)
   static std::string getInstanceName(const std::string& layer_name, int32_t index);

   //! the layer name without its instance suffix
   static std::string getInstanceBaseName(const std::string& layer_name);

   //! send out an action request
   Signal<const std::string&, const std::string&> actionRequestSignal;

   //! a key was pressed while an item was focussed
   Signal<const std::string&, const std::string&, int> actionKeyPressedSignal;

   //! a layer has been focussed
   Signal<const std::string&, const std::string&> layerFocussedSignal;

   void mouseMoved(int x, int y);

   void mousePressed(int x, int y);

   void paste(const std::string& text);

   void mouseReleased();

   void keyPressed(int key, const std::string& text);

   void deactivate();

   void resetAnimation();

   void unFocusAllItems();

   //! render to framebuffer
   void render();

protected:
   //! send out an action request
   void actionRequestFromItem(const std::string& action_request);

   //! initialize layers
   void initializeLayers();

   //! clones the layer groups listed in the page's repeat_groups setting
   void repeatGroups();

   //! initialize page items
   void initializePageItems();

   //! initialize tab indices
   void initializeTabIndices();

   //! tab pressed
   void tabPressed();

   //! creates a page item owned by this page
   template <typename T>
   T& addPageItem();

   //! adds or replaces the page item of that name
   void setPageItemName(const std::string& name, MenuPageItem& item);

   //! a nullopt result is not initialized as page item
   std::optional<std::reference_wrapper<MenuPageItem>> processLabel(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processLineEdit(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processBackground(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processTableMain(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processTableScrollButtons(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processTableScrollBar(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processTableScrollBarSlider(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processSliderScrollBarIcons(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processScrollImage(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processCheckBox(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processPixmap(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processDefaultItem(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processButton(PSDLayer& layer, const std::string& layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processComboBox(PSDLayer& layer, std::string layer_name);
   std::optional<std::reference_wrapper<MenuPageItem>> processEditableComboBox(PSDLayer& layer, std::string layer_name);

   //! a combobox opens when its button is clicked
   void linkComboBoxToButton(const std::string& button_key, const std::string& combo_box_key);

   //! a combobox shows its value in a label
   void linkComboBoxToLabel(const std::string& label_key, const std::string& combo_box_key);

   //! an editable combobox shows its value in a text edit
   void linkComboBoxToTextEdit(const std::string& text_edit_key, const std::string& combo_box_key);

   //! the active item is this one
   bool isActiveItem(const MenuPageItem& item) const;

   // declared before the items so the items (which observe the layers) are destroyed first
   std::vector<std::unique_ptr<PSDLayer>> _render_layers;

   std::vector<std::unique_ptr<MenuPageItem>> _page_items;

   // lookup into _page_items
   std::map<std::string, std::reference_wrapper<MenuPageItem>> _page_item_name_map;

   std::string _title;

   std::string _filename;

   std::unique_ptr<Settings> _settings;

   //! focussed item
   std::optional<std::reference_wrapper<MenuPageItem>> _active_item;

   //! page animation, owned by MenuDrawable
   std::optional<std::reference_wrapper<MenuPageAnimation>> _animation;

   //! page is active
   bool _active = false;

   //! repeated group -> layer indices of each instance
   std::map<std::string, std::vector<std::vector<size_t>>> _group_instances;

   //! repeated group -> current horizontal offset of each instance
   std::map<std::string, std::vector<int32_t>> _group_instance_offsets;
};

template <typename T>
std::optional<std::reference_wrapper<T>> MenuPage::getPageItem(const std::string& layer_name) const
{
   const auto iterator = _page_item_name_map.find(layer_name);
   if (iterator == _page_item_name_map.end())
   {
      return std::nullopt;
   }

   if constexpr (std::is_same_v<T, MenuPageItem>)
   {
      return iterator->second;
   }
   else
   {
      if (auto* item = dynamic_cast<T*>(&iterator->second.get()))
      {
         return *item;
      }
      return std::nullopt;
   }
}
