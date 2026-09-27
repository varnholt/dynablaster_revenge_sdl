#pragma once

// shared
#include "settings.h"
#include "signal.h"

// menus
#include "image/psd.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

// forward declarations
class MenuPageItem;
class MenuPageAnimation;
class PSDLayer;

/// \brief GLES3-era port of client/src/menus/menupage.cpp.
///
/// initializePageItems()'s dispatch covers every branch any of the 9 real menu pages actually use
/// (confirmed by scanning all 9 PSDs' real layer names - see project memory): combobox,
/// editablecombobox, checkbox, button, label, lineedit, background, pixmap, table_*_main/
/// scroll_up/scroll_down/scrollbar/scroll_slider, slider_, image_scroll, plus the catch-all
/// default item. Not ported at all, because MenuPage::initializePageItems() never dispatches to
/// them for ANY real page (confirmed: no "radio"/"colorselect" dispatch exists in the original
/// menupage.cpp either - genuinely dead code upstream): MenuPageRadioButtonItem,
/// MenuPageColorSelectItem.
///
/// The QRegExp-driven "*_input_regexp" ini key is no longer read - MenuPageTextEditItem and
/// MenuPageListItem dropped setRegExp()/mRegexp entirely (QRegExp doesn't exist in Qt6, and the
/// field was dead upstream anyway - see those classes).
class MenuPage : public PSD
{
public:
   MenuPage();

   virtual ~MenuPage();

   void setTitle(const std::string&);

   void setFilename(const std::string&);

   std::string getFilename() const;

   void initialize();

   std::vector<MenuPageItem*>* getPageItems();

   void setActive(bool);

   bool isActive();

   void setAnimation(MenuPageAnimation*);

   MenuPageAnimation* getAnimation();

   MenuPageItem* getPageItem(const std::string& layerName) const;

   MenuPageItem* processLabel(PSDLayer* layer, std::string layerName);

   MenuPageItem* processLineEdit(PSDLayer* layer, std::string layerName);

   MenuPageItem* processBackground(PSDLayer* layer, std::string layerName);

   MenuPageItem* processTableMain(PSDLayer* layer, std::string layerName);

   MenuPageItem* processTableScrollButtons(PSDLayer* layer, std::string layerName);

   MenuPageItem* processTableScrollBar(PSDLayer* layer, std::string layerName);

   MenuPageItem* processTableScrollBarSlider(PSDLayer* layer, std::string layerName);

   MenuPageItem* processSliderScrollBarIcons(PSDLayer* layer, std::string layerName);

   MenuPageItem* processScrollImage(PSDLayer* layer, std::string layerName);

   MenuPageItem* processCheckBox(PSDLayer* layer, std::string layerNameWithoutPostfix, std::string layerName);

   MenuPageItem* processPixmap(PSDLayer* layer, std::string layerName);

   MenuPageItem* processDefaultItem(PSDLayer* layer, std::string layerName);

   MenuPageItem* processButton(PSDLayer* layer, std::string layerName, std::string layerNameWithoutPostfix);

   MenuPageItem* processComboBox(PSDLayer* layer, std::string layerName);

   MenuPageItem* processEditableComboBox(PSDLayer* layer, std::string layerName);

   //! getter for the active item
   MenuPageItem* getActiveItem() const;

   //! setter for the active item
   void setActiveItem(MenuPageItem* value);

   //! getter for pageitem at given position
   std::vector<MenuPageItem*> getItemsAt(int x, int y) const;

   //! getter for the focussed item
   MenuPageItem* getFocussedItem() const;

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
   void actionRequestFromItem(const std::string& actionRequest);

   //! initialize layers
   void initializeLayers();

   //! initialize page items
   void initializePageItems();

   //! initialize tab indices
   void initializeTabIndices();

   //! tab pressed
   void tabPressed();

   // page item information

   std::vector<MenuPageItem*> mPageItems;

   std::map<std::string, MenuPageItem*> mPageItemNameMap;

   std::vector<PSDLayer*> mRenderLayers;

   // page information

   std::string mTitle;

   std::string mFilename;

   std::unique_ptr<Settings> mSettings;

   //! focussed item
   MenuPageItem* mActiveItem;

   //! page animation
   MenuPageAnimation* mAnimation;

   //! page is active
   bool mActive;
};
