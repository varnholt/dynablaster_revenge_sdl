#pragma once

#include <cstdint>
#include <string>
#include <vector>

class GlesContext;

/// \brief the choices on the video options page, and the window side of the video settings:
/// display mode, swap interval and the fps shown in the window title.
class VideoOptions
{
public:
   explicit VideoOptions(const GlesContext& context);
   VideoOptions(const VideoOptions&) = delete;
   VideoOptions& operator=(const VideoOptions&) = delete;

   /// \brief render resolution divisors, one per 320 desktop pixels (1 = full resolution)
   static std::vector<int32_t> resolutions();

   /// \brief 1 plus the multisample counts the GPU supports, ascending
   static std::vector<int32_t> sampleCounts();

   /// \brief applies fullscreen, vsync and the fps title from GameSettings
   void apply();

   /// \brief Alt+Enter: flips fullscreen and stores it
   void toggleFullscreen();

   /// \brief remembers a windowed size change, the next start opens at that size
   void storeWindowSize();

   /// \brief counts a presented frame, refreshes the fps title once a second
   void frameSwapped();

private:
   void applyFullscreen();

   const GlesContext& _context;
   std::string _title;
   bool _show_fps = false;
   uint64_t _fps_start_ticks = 0;
   int32_t _frames = 0;
};
