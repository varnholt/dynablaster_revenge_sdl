#include "videooutput.h"

#include "gamesettings.h"

#include "framework/framebuffer.h"
#include "framework/gldevice.h"

#include <algorithm>
#include <cmath>

VideoOutput::VideoOutput(GLDevice& device) : _device(device)
{
   _brightness.init();
}

VideoOutput::~VideoOutput()
{
   FrameBuffer::clearScreen();
}

void VideoOutput::beginFrame(int32_t window_width, int32_t window_height)
{
   _window_width = std::max(window_width, 1);
   _window_height = std::max(window_height, 1);

   auto& video_settings = GameSettings::getInstance().getVideoSettings();

   // the frame: window / resolution divisor, cut to 16:9
   const auto resolution = std::max(video_settings.getResolution(), 1);
   auto width = (_window_width + resolution - 1) / resolution;
   auto height = (_window_height + resolution - 1) / resolution;

   if (((width * 9) >> 4) >= height)
   {
      width = (height << 4) / 9;
   }
   else
   {
      height = (width * 9) >> 4;
   }

   width = std::max(width, 1);
   height = std::max(height, 1);

   const auto samples = video_settings.getAntialias();

   if (!_frame)
   {
      _frame = std::make_unique<FrameBuffer>(width, height, samples);
      FrameBuffer::setScreen(*_frame);
   }
   else if (_frame->resolutionChanged(width, height, samples))
   {
      _frame->setResolution(width, height, samples);
   }

   // what the driver actually gave, like updateFrameBufferSettings()
   if (samples > 1 && _frame->samples() != samples)
   {
      video_settings.setAntialias(_frame->samples());
   }

   // its place in the window: pillarbox if the window is wider than 16:9, letterbox otherwise
   const auto pillar_width = (_window_height << 4) / 9;
   if (_window_width > pillar_width)
   {
      _viewport = {(_window_width - pillar_width) >> 1, 0, pillar_width, _window_height};
   }
   else
   {
      const auto letter_height = (_window_width * 9) >> 4;
      _viewport = {0, (_window_height - letter_height) >> 1, _window_width, letter_height};
   }

   _viewport.width = std::max(_viewport.width, 1);
   _viewport.height = std::max(_viewport.height, 1);

   _frame->bind();
   _device.resize(width, height);
}

void VideoOutput::endFrame()
{
   _frame->unbind();

   // the bars
   _device.resize(_window_width, _window_height);
   _device.clear();

   _device.setViewPort(_viewport.x, _viewport.y, _viewport.width, _viewport.height);

   // whole pixel scale factors, so the scaled up frame keeps square pixels
   const auto scale_x = static_cast<int32_t>(std::floor(static_cast<float>(_viewport.width) / _frame->width() + 0.5f));
   const auto scale_y = static_cast<int32_t>(std::floor(static_cast<float>(_viewport.height) / _frame->height() + 0.5f));

   auto u = 1.0f;
   auto v = 1.0f;
   if (_viewport.width >= _frame->width() && _viewport.height >= _frame->height())
   {
      u = static_cast<float>(_viewport.width) / static_cast<float>(_frame->width() * scale_x);
      v = static_cast<float>(_viewport.height) / static_cast<float>(_frame->height() * scale_y);
   }

   const auto level = GameSettings::getInstance().getVideoSettings().getBrightness();
   _brightness.setGamma(2.2f - (level - 0.5f));
   _brightness.process(_frame->texture(), u, v);
}

void VideoOutput::toPageSpace(int32_t& x, int32_t& y) const
{
   // window y runs down, the viewport's up
   const auto top = _window_height - (_viewport.y + _viewport.height);

   x = ((x - _viewport.x) * 1920) / _viewport.width;
   y = ((y - top) * 1080) / _viewport.height;
}
