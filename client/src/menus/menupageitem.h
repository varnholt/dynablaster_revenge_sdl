#pragma once

#include "psdlayer.h"
#include "gamesignal.h"

#include <string>

class MenuPageItem
{
public:
   enum PageItemType
   {
      PageItemTypeUnknown,
      PageItemTypeButton,
      PageItemTypeCheckbox,
      PageItemTypeCombobox,
      PageItemTypeEditableCombobox,
      PageItemTypeLabel,
      PageItemTypeList,
      PageItemTypeListElement,
      PageItemTypePixmap,
      PageItemTypeRadioButton,
      PageItemTypeScrollbar,
      PageItemTypeScrollImage,
      PageItemTypeSlider,
      PageItemTypeTextedit
   };

   MenuPageItem() = default;
   virtual ~MenuPageItem() = default;

   PageItemType getPageItemType() const;

   // layer information

   virtual void setActiveLayer(PSDLayer* layer);

   virtual void setInactiveLayer(PSDLayer* layer);

   virtual PSDLayer* getLayer() const;

   virtual PSD::Layer* getCurrentLayer();

   virtual PSDLayer* getActiveLayer();

   virtual PSDLayer* getInactiveLayer();

   // main

   //! initialize menupage item
   virtual void initialize();

   //! draw menupage item
   virtual void draw();

   // behaviour

   //! setter for interactive flag
   void setInteractive(bool);

   //! getter for interactive flag
   bool isInteractive();

   //! the item's PSD layer is drawn, text items may use it as background only optionally
   void setLayerDrawn(bool drawn);
   bool isLayerDrawn() const;

   //! menupage item is focussed
   bool isFocussed() const;

   //! menu page item is modal
   virtual bool isModal() const;

   //! setter for action identifier
   void setAction(const std::string&);

   //! menu page item has integrated items
   virtual bool hasNestedElements();

   //! menu page item is grabbing mouse events
   virtual bool isGrabbingMouseEvents();

   //! check if item is active
   bool isActive();

   //! getter for visible flag
   bool isVisible() const;

   //! check if item is enabled
   bool isEnabled() const;

   //! setter for tab index
   void setTabIndex(int tab_index);

   //! getter for tab index
   int getTabIndex() const;

   //! check if action request on click is enabled
   virtual bool isActionRequestOnClickEnabled() const;

   // event handlers

   //! mouse moved
   virtual void mouseMoved(int x, int y);

   //! mouse pressed
   virtual void mousePressed(int x, int y);

   //! mouse released
   virtual void mouseReleased();

   //! getter for parent item
   MenuPageItem* getParent() const;

   //! setter for parent item
   void setParent(MenuPageItem* value);

   //! action was triggered
   Signal<const std::string&> actionSignal;

   //! mouse was released
   Signal<> mouseReleasedSignal;

   //! visibility changed
   Signal<bool> visibleSignal;

   //! setter for visible flag
   void setVisible(bool);

   //! item was set active
   virtual void setActive(bool);

   virtual void setFocus(bool);

   virtual void activated();

   virtual void deactivated();

   virtual void keyPressed(int key, const std::string& text);

   virtual void paste(const std::string& /*text*/)
   {
   }

   virtual void animate(float /*time*/)
   {
   }

   virtual void setEnabled(bool enabled);

protected:
   PageItemType _page_item_type = PageItemTypeUnknown;

   std::string _action;

   // non-owning, the layers belong to the MenuPage
   PSDLayer* _layer_active = nullptr;
   PSDLayer* _layer_inactive = nullptr;

   bool _focussed = false;
   bool _interactive = false;
   bool _active = false;
   bool _visible = true;

   bool _layer_drawn = true;
   bool _enabled = true;

   int _tab_index = -1;

   MenuPageItem* _parent = nullptr;
};
