#pragma once

#include "gamesignal.h"
#include "menupageitem.h"

class MenuPageCheckBoxItem : public MenuPageItem
{
public:
   MenuPageCheckBoxItem();

   void draw() override;

   virtual void setCheckedLayer(PSDLayer& layer);

   virtual void setUncheckedLayer(PSDLayer& layer);

   virtual std::optional<std::reference_wrapper<PSDLayer>> getCheckedLayer() const;

   virtual std::optional<std::reference_wrapper<PSDLayer>> getUncheckedLayer() const;

   std::optional<std::reference_wrapper<PSDLayer>> getLayer() const override;

   bool isChecked() const;

   void setChecked(bool checked);

   void activated() override;

   void deactivated() override;

   Signal<> stateChangedSignal;

protected:
   virtual void toggleChecked();

   // the layers belong to the MenuPage
   std::optional<std::reference_wrapper<PSDLayer>> _layer_checked;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_unchecked;

   bool _checked = false;
};
