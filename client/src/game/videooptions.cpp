#include "videooptions.h"

#include "gamesettings.h"
#include "gles3.h"
#include "glescontext.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <format>

VideoOptions::VideoOptions(const GlesContext& context)
    : _context(context)
{
   if (const char* title = SDL_GetWindowTitle(_context.window()))
   {
      _title = title;
   }
}

std::vector<int32_t> VideoOptions::resolutions()
{
   int32_t width = 1920;
   int32_t height = 1080;

   if (const SDL_DisplayMode* mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay()))
   {
      width = mode->w;
      height = mode->h;
   }

   // two monitors side by side count as one
   if (width >= 3 * height)
   {
      width /= 2;
   }

   std::vector<int32_t> divisors;
   for (int32_t factor = 1; width / factor > 320; factor++)
   {
      divisors.push_back(factor);
   }

   if (divisors.empty())
   {
      divisors.push_back(1);
   }

   return divisors;
}

std::vector<int32_t> VideoOptions::sampleCounts()
{
   std::vector<int32_t> counts{1};

   GLint count = 0;
   glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, GL_NUM_SAMPLE_COUNTS, 1, &count);

   if (count > 0)
   {
      std::vector<GLint> samples(static_cast<size_t>(count));
      glGetInternalformativ(GL_RENDERBUFFER, GL_RGBA8, GL_SAMPLES, count, samples.data());
      counts.insert(counts.end(), samples.begin(), samples.end());
   }

   std::ranges::sort(counts);
   const auto [first, last] = std::ranges::unique(counts);
   counts.erase(first, last);

   return counts;
}

void VideoOptions::apply()
{
   const auto* settings = GameSettings::getInstance()->getVideoSettings();

   applyFullscreen();

   // 0: off, 1: every refresh (60fps), 2: every other refresh (30fps)
   if (!SDL_GL_SetSwapInterval(std::clamp(settings->getVSync(), 0, 2)))
   {
      SDL_Log("VideoOptions: swap interval %d failed: %s", settings->getVSync(), SDL_GetError());
   }

   _show_fps = settings->isFpsShown();
   _fps_start_ticks = SDL_GetTicks();
   _frames = 0;

   if (!_show_fps)
   {
      SDL_SetWindowTitle(_context.window(), _title.c_str());
   }
}

void VideoOptions::toggleFullscreen()
{
   auto* settings = GameSettings::getInstance()->getVideoSettings();
   settings->toggleFullscreen();
   settings->serialize();

   // Cancel on the video page mustn't bring back the old display mode
   GameSettings::getInstance()->getVideoSettingsBackup()->setFullscreen(settings->isFullscreen());

   applyFullscreen();
}

void VideoOptions::storeWindowSize()
{
   if ((SDL_GetWindowFlags(_context.window()) & SDL_WINDOW_FULLSCREEN) != 0)
   {
      return;
   }

   int width = 0;
   int height = 0;
   if (!SDL_GetWindowSize(_context.window(), &width, &height))
   {
      return;
   }

   auto* settings = GameSettings::getInstance()->getVideoSettings();
   if (settings->getWidth() != width || settings->getHeight() != height)
   {
      settings->setWidth(width);
      settings->setHeight(height);
      settings->serialize();
   }
}

void VideoOptions::frameSwapped()
{
   if (!_show_fps)
   {
      return;
   }

   _frames++;

   const auto now = SDL_GetTicks();
   const auto elapsed = now - _fps_start_ticks;
   if (elapsed >= 1000)
   {
      const auto fps = _frames * 1000.0f / static_cast<float>(elapsed);
      SDL_SetWindowTitle(_context.window(), std::format("{} - fps: {:.2f}", _title, fps).c_str());

      _frames = 0;
      _fps_start_ticks = now;
   }
}

void VideoOptions::applyFullscreen()
{
   const bool fullscreen = GameSettings::getInstance()->getVideoSettings()->isFullscreen();
   const bool is_fullscreen = (SDL_GetWindowFlags(_context.window()) & SDL_WINDOW_FULLSCREEN) != 0;

   if (fullscreen != is_fullscreen && !SDL_SetWindowFullscreen(_context.window(), fullscreen))
   {
      SDL_Log("VideoOptions: fullscreen %d failed: %s", fullscreen, SDL_GetError());
   }
}
