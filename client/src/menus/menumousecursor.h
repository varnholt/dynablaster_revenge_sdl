#pragma once

#include "framework/drawable.h"
#include "framework/frametimer.h"

#include "image/psd.h"

#include <memory>
#include <string>

class PSDLayer;

class MenuMouseCursor : public Drawable
{
public:
   enum State
   {
      Default,
      Clicked,
      Busy
   };

   MenuMouseCursor(RenderDevice* device, bool visible = false);
   ~MenuMouseCursor() override;

   void initializeGL() override;
   void paintGL() override;
   void animate(float global_time) override;

   void mousePressEvent(int x, int y) override;
   void mouseMoveEvent(int x, int y) override;
   void mouseReleaseEvent() override;

   void setBusy(bool);

protected:
   void initializeLayers();

   void initGlParameters();
   void cleanupGlParameters();

   void paintDefaultCursor();
   void paintClickedCursor();
   void paintCursor(PSDLayer* layer, float opacity = 1.0f);
   void paintBusyIcon();

   // declared before the layers, which observe its PSD::Layers
   PSD _psd;

   std::string _filename = "data/cursors/cursor_small.psd";

   std::unique_ptr<PSDLayer> _default_layer;
   std::unique_ptr<PSDLayer> _clicked_layer;
   std::unique_ptr<PSDLayer> _busy_layer;

   int _busy_x = 0;
   int _busy_y = 0;

   int _x = 0;
   int _y = 0;

   bool _busy = false;
   bool _mouse_pressed = false;

   FrameTimer _click_time;

   float _time = 0.0f;
};
