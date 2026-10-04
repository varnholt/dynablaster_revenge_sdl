#pragma once

#include "drawable.h"

#include "framework/frametimer.h"
#include "image/psd.h"

#include <memory>
#include <vector>

class PSDLayer;

/// \brief F1 overlay listing the hotkeys and extras (data/game/help.psd); any key or click fades
/// it out again
class HotkeyDrawable : public Drawable
{
public:
   explicit HotkeyDrawable(RenderDevice& dev);
   ~HotkeyDrawable() override;

   void initializeGL() override;
   void paintGL() override;
   void setVisible(bool visible) override;

   //! \c false while fading out
   bool isShown() const;

   //! F1: shows the overlay or fades it out
   void toggle();

protected:
   void initGlParameters();
   void cleanupGlParameters();
   float computeAlpha() const;

   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;
   FrameTimer _animation_timer;
   bool _show = false;
};
