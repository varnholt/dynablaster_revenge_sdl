#include "glescontext.h"

#include "gles3.h"

#ifdef __SWITCH__
#include <switch.h>
#endif

bool GlesContext::init(const std::string& title, int width, int height)
{
   if (!SDL_Init(SDL_INIT_VIDEO))
   {
      SDL_Log("SDL_Init failed: %s", SDL_GetError());
      return false;
   }

   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

#ifdef __SWITCH__
   // SDL caches the request, but the shared libnx window owns the framebuffer.
   // Its size also differs from the docked display mode (1080p scan-out).
   u32 native_width = 0;
   u32 native_height = 0;
   if (R_FAILED(nwindowGetDimensions(nwindowGetDefault(), &native_width, &native_height)) ||
       native_width == 0 || native_height == 0)
   {
      SDL_Log("Could not determine the Switch framebuffer size");
      return false;
   }
   width = static_cast<int>(native_width);
   height = static_cast<int>(native_height);
#endif

   _window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_OPENGL);
   if (_window == nullptr)
   {
      SDL_Log("SDL_CreateWindow failed: %s", SDL_GetError());
      return false;
   }

   // windows' focus-stealing prevention can leave a window launched from a console without input
   // focus, so claim it explicitly
   SDL_RaiseWindow(_window);

   _context = SDL_GL_CreateContext(_window);
   if (_context == nullptr)
   {
      SDL_Log("SDL_GL_CreateContext failed: %s", SDL_GetError());
      return false;
   }

   if (!loadGles3Functions())
   {
      SDL_Log("failed to resolve one or more GLES3 entry points");
      return false;
   }

   SDL_GL_SetSwapInterval(1);

   // Rendering and pointer conversion need the actual framebuffer size.
   updateSize();

   return true;
}

void GlesContext::swap() const
{
   SDL_GL_SwapWindow(_window);
}

void GlesContext::updateSize()
{
#ifdef __SWITCH__
   u32 width = 0;
   u32 height = 0;
   if (R_SUCCEEDED(nwindowGetDimensions(nwindowGetDefault(), &width, &height)) && width > 0 && height > 0)
   {
      _width = width;
      _height = height;
      return;
   }
#endif
   SDL_GetWindowSizeInPixels(_window, &_width, &_height);
}

GlesContext::~GlesContext()
{
   if (_context != nullptr)
   {
      SDL_GL_DestroyContext(_context);
   }

   if (_window != nullptr)
   {
      SDL_DestroyWindow(_window);
   }

   SDL_Quit();
}
