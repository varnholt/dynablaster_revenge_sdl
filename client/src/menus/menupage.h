#pragma once

#include "settings.h"
#include "gamesignal.h"

#include "image/psd.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

class MenuPageItem;
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

   void setAnimation(MenuPageAnimation*);

   MenuPageAnimation* getAnimation();

   MenuPageItem* getPageItem(const std::string& layer_name) const;

   MenuPageItem* processLabel(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processLineEdit(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processBackground(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processTableMain(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processTableScrollButtons(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processTableScrollBar(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processTableScrollBarSlider(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processSliderScrollBarIcons(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processScrollImage(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processCheckBox(PSDLayer* layer, std::string layer_name_without_postfix, std::string layer_name);

   MenuPageItem* processPixmap(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processDefaultItem(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processButton(PSDLayer* layer, std::string layer_name, std::string layer_name_without_postfix);

   MenuPageItem* processComboBox(PSDLayer* layer, std::string layer_name);

   MenuPageItem* processEditableComboBox(PSDLayer* layer, std::string layer_name);

   //! getter for the active item
   MenuPageItem* getActiveItem() const;

   //! setter for the active item
   void setActiveItem(MenuPageItem* value);

   //! getter for pageitem at given position
   std::vector<MenuPageItem*> getItemsAt(int x, int y) const;

   //! getter for the focussed item
   MenuPageItem* getFocussedItem() const;

   //! a repeated layer group (see repeatGroups()) has this many instances, 0 if it isn't one
   int32_t getGroupInstanceCount(const std::string& group) const;

   //! moves one instance of a repeated group horizontally, relative to where the PSD has it
   void setGroupInstanceOffset(const std::string& group, int32_t index, int32_t offset);

   //! name of a layer's copy in a repeated group's instance (1-based)
   static std::string getInstanceName(const std::string& layer_name, int32_t index);

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
   T* addPageItem();

   // declared before the items so the items (which observe the layers) are destroyed first
   std::vector<std::unique_ptr<PSDLayer>> _render_layers;

   std::vector<std::unique_ptr<MenuPageItem>> _page_items;

   // non-owning lookup into _page_items
   std::map<std::string, MenuPageItem*> _page_item_name_map;

   std::string _title;

   std::string _filename;

   std::unique_ptr<Settings> _settings;

   //! focussed item (non-owning)
   MenuPageItem* _active_item = nullptr;

   //! page animation (non-owning, owned by MenuDrawable)
   MenuPageAnimation* _animation = nullptr;

   //! page is active
   bool _active = false;

   //! repeated group -> layer indices of each instance
   std::map<std::string, std::vector<std::vector<size_t>>> _group_instances;

   //! repeated group -> current horizontal offset of each instance
   std::map<std::string, std::vector<int32_t>> _group_instance_offsets;
};
