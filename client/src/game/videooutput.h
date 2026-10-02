#pragma once

#include "postproduction/brightnessfilter.h"

#include <cstdint>
#include <memory>

class FrameBuffer;
class GLDevice;

/// \brief draws each frame offscreen at the configured render resolution and presents it 16:9,
/// pillar- or letterboxed, scaled up and with the configured brightness (GameView::paintGL()).
class VideoOutput
{
public:
   explicit VideoOutput(GLDevice& device);
   ~VideoOutput();
   VideoOutput(const VideoOutput&) = delete;
   VideoOutput& operator=(const VideoOutput&) = delete;

   /// \brief binds the frame, the device reports its size until endFrame()
   void beginFrame(int32_t window_width, int32_t window_height);

   /// \brief draws the frame into the window
   void endFrame();

   /// \brief window pixels to the menu's 1920x1080 page space
   void toPageSpace(int32_t& x, int32_t& y) const;

private:
   struct Rect
   {
      int32_t x = 0;
      int32_t y = 0;  //!< from the bottom, like glViewport
      int32_t width = 1;
      int32_t height = 1;
   };

   GLDevice& _device;
   std::unique_ptr<FrameBuffer> _frame;
   BrightnessFilter _brightness;

   int32_t _window_width = 1;
   int32_t _window_height = 1;
   Rect _viewport;  //!< where the frame ends up in the window
};
