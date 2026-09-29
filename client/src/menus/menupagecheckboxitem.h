#pragma once

#include "menupageitem.h"
#include "signal.h"

class MenuPageCheckBoxItem : public MenuPageItem
{
public:
   MenuPageCheckBoxItem();

   void draw() override;

   virtual void setCheckedLayer(PSDLayer* layer);

   virtual void setUncheckedLayer(PSDLayer* layer);

   virtual PSDLayer* getCheckedLayer() const;

   virtual PSDLayer* getUncheckedLayer() const;

   PSDLayer* getLayer() const override;

   bool isChecked() const;

   void setChecked(bool checked);

   void activated() override;

   void deactivated() override;

   Signal<> stateChangedSignal;

protected:
   virtual void toggleChecked();

   // non-owning, the layers belong to the MenuPage
   PSDLayer* _layer_checked = nullptr;
   PSDLayer* _layer_unchecked = nullptr;

   bool _checked = false;
};
