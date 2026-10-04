#pragma once

#include "drawable.h"

#include "constants.h"
#include "helpelement.h"
#include "image/psd.h"

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class BitmapFont;
class PSDLayer;

/// \brief the menu's help and error toasts: a sprite slides in from the screen edge and pops up a
/// speech bubble with up to three lines (';' separated) - fed by HelpManager
class GameHelpDrawable : public Drawable
{
public:
   explicit GameHelpDrawable(RenderDevice& dev);
   ~GameHelpDrawable() override;

   void initializeGL() override;
   void paintGL() override;

   void showHelp(
      const std::string& page,
      const std::string& message,
      Constants::HelpSeverity severity,
      Constants::HelpLocation location,
      int delay = 0
   );

   //! toasts bound to another page are dropped once they're due
   void pageChanged(const std::string& page);

protected:
   void initializeLayers();
   void initGlParameters();
   void cleanupGlParameters();
   void updateHelpItems();
   void drawQuad(PSDLayer& layer, float left, float top, float alpha, float scale_x = 0.0f, float scale_y = 0.0f);
   void playSound(const HelpElement& element);

   PSD _psd;
   std::vector<std::unique_ptr<PSDLayer>> _psd_layers;

   std::optional<std::reference_wrapper<PSDLayer>> _layer_icon_info;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_sprite_info;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_icon_error;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_sprite_error;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_bubble;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_line_title;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_line_2;
   std::optional<std::reference_wrapper<PSDLayer>> _layer_line_3;

   std::optional<std::reference_wrapper<BitmapFont>> _font;

   std::deque<HelpElement> _help_items;
   std::string _page;
};
