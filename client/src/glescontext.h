#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <memory>
#include <string>

/// \brief owns the SDL window and its GLES 3.0 context.
class GlesContext
{
public:
   GlesContext() = default;
   ~GlesContext();

   GlesContext(const GlesContext&) = delete;
   GlesContext& operator=(const GlesContext&) = delete;

   /// \brief initializes SDL video, creates the window and a GLES 3.0 context.
   /// \param title window title.
   /// \param width window width in pixels.
   /// \param height window height in pixels.
   /// \return true on success.
   bool init(const std::string& title, int width, int height);

   /// \brief presents the back buffer.
   void swap() const;

   /// \brief re-queries the window's actual drawable pixel size - call after
   /// SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED (e.g. fullscreen toggle, user resize) so width()/height()
   /// stay correct; nothing does this automatically otherwise, since they're only set once in
   /// init().
   void updateSize();

   int width() const
   {
      return _width;
   }

   int height() const
   {
      return _height;
   }

   SDL_Window* window() const
   {
      return _window.get();
   }

private:
   struct WindowDeleter
   {
      void operator()(SDL_Window* window) const
      {
         SDL_DestroyWindow(window);
      }
   };

   struct ContextDeleter
   {
      void operator()(SDL_GLContext context) const
      {
         SDL_GL_DestroyContext(context);
      }
   };

   // the context is released before the window
   std::unique_ptr<SDL_Window, WindowDeleter> _window;
   std::unique_ptr<SDL_GLContextState, ContextDeleter> _context;
   int _width = 0;
   int _height = 0;
};
