#pragma once

#include <cstdint>

#include "keyevent.h"

class RenderDevice;

// base class for everything main.cpp renders and forwards input events to
class Drawable
{
public:
   Drawable(RenderDevice* device, bool visible = false);
   virtual ~Drawable() = default;

   virtual void initializeGL() = 0;
   virtual void paintGL() = 0;
   virtual void resizeGL();

   virtual void animate(float global_time);

   virtual void mousePressEvent(int32_t x, int32_t y);
   virtual void mouseMoveEvent(int32_t x, int32_t y);
   virtual void mouseReleaseEvent();

   virtual void keyPressEvent(const KeyEvent& event);
   virtual void keyReleaseEvent(const KeyEvent& event);

   virtual void setVisible(bool visible);
   virtual bool isVisible() const;

protected:
   // kept as mDevice/mVisible: derived classes outside framework/ access them directly
   RenderDevice* mDevice = nullptr;  // render device (not owned)
   bool mVisible = false;            // visibility flag
};
