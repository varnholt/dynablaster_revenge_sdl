#include "gles3.h"
#include "glescontext.h"
#include "framework/gldevice.h"
#include "engine/materials/material.h"
#include "image/image.h"

#include <SDL3/SDL.h>

#include <array>
#include <vector>

namespace
{
bool checkError(const char* operation)
{
   const GLenum error = glGetError();
   if (error != GL_NO_ERROR)
   {
      SDL_Log("%s: GL error 0x%x", operation, error);
      return false;
   }
   return true;
}

// Read the actual GLES texture through an FBO, avoiding desktop-only glGetTexImage.
bool checkTexture(GLuint texture, GLenum target, int level, const Image& expected)
{
   GLuint framebuffer = 0;
   glGenFramebuffers(1, &framebuffer);
   glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target, texture, level);
   bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
   if (ok)
   {
      const size_t count = static_cast<size_t>(expected.getWidth()) * expected.getHeight();
      std::vector<uint8_t> pixels(count * 4);
      glReadPixels(0, 0, expected.getWidth(), expected.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
      for (size_t i = 0; i < count; ++i)
      {
         const uint32_t argb = expected.getData()[i];
         if (pixels[i * 4] != ((argb >> 16) & 255) || pixels[i * 4 + 1] != ((argb >> 8) & 255)
             || pixels[i * 4 + 2] != (argb & 255) || pixels[i * 4 + 3] != (argb >> 24))
         {
            SDL_Log("Texture 0x%x mip %d pixel %zu: channel or orientation mismatch", target, level, i);
            ok = false;
            break;
         }
      }
   }
   else
   {
      SDL_Log("Texture 0x%x mip %d: incomplete framebuffer", target, level);
   }
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glDeleteFramebuffers(1, &framebuffer);
   return checkError("texture readback") && ok;
}
}  // namespace

int main()
{
   GlesContext context;
   if (!context.init("GLES compatibility test", 64, 64))
      return 1;

   SDL_Log("GL_VERSION: %s", glGetString(GL_VERSION));
   SDL_Log("GL_RENDERER: %s", glGetString(GL_RENDERER));

   GLDevice device;
   if (!device.init() || !checkError("GLDevice::init"))
      return 1;

   // Distinct red/blue and nonopaque alpha detect accidental BGRA uploads.
   Image palette(4, 1);
   palette.getData()[0] = 0x801122ee;
   palette.getData()[1] = 0x904433cc;
   palette.getData()[2] = 0xa06655aa;
   palette.getData()[3] = 0xb0887799;
   const GLuint palette_texture = device.createTexture(palette.getData(), 4, 1, 1 | 4);
   bool ok = checkError("palette upload") && checkTexture(palette_texture, GL_TEXTURE_2D, 0, palette);
   glDeleteTextures(1, &palette_texture);

   // A different pattern in each cross cell catches swapped faces and rotations.
   constexpr int face_size = 4;
   Image cross(face_size * 3, face_size * 4);
   for (int y = 0; y < cross.getHeight(); ++y)
   {
      for (int x = 0; x < cross.getWidth(); ++x)
      {
         cross.getScanline(y)[x] = (static_cast<uint32_t>(128 + x + y) << 24)
                                  | (static_cast<uint32_t>(16 + x * 3) << 16)
                                  | (static_cast<uint32_t>(32 + y * 5) << 8) | (240 - x - y);
      }
   }
   const std::vector<uint32_t> original(cross.getData(), cross.getData() + cross.getWidth() * cross.getHeight());
   const GLuint cube = Material::uploadCubeMap(cross);
   ok = checkError("cubemap upload") && cube != 0 && ok;

   constexpr std::array<GLenum, 6> targets = {
      GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
      GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
   };
   constexpr std::array<std::array<int, 2>, 6> cells = {{{2, 1}, {0, 1}, {1, 0}, {1, 2}, {1, 1}, {1, 3}}};
   for (size_t side = 0; side < targets.size(); ++side)
   {
      Image expected(face_size, face_size);
      for (int y = 0; y < face_size; ++y)
      {
         for (int x = 0; x < face_size; ++x)
         {
            const int sx = cells[side][0] * face_size + (side == 5 ? face_size - 1 - x : x);
            const int sy = cells[side][1] * face_size + (side == 5 ? face_size - 1 - y : y);
            expected.getScanline(y)[x] = original[sy * cross.getWidth() + sx];
         }
      }
      for (int level = 0; expected.getWidth() > 0; ++level)
      {
         ok = checkTexture(cube, targets[side], level, expected) && ok;
         expected = expected.downsample();
      }
   }
   for (size_t i = 0; i < original.size(); ++i)
   {
      if (cross.getData()[i] != original[i])
      {
         SDL_Log("Cubemap upload changed source pixel %zu", i);
         ok = false;
         break;
      }
   }
   glDeleteTextures(1, &cube);
   SDL_Log("GLES compatibility: %s", ok ? "PASS" : "FAIL");
   return ok ? 0 : 1;
}
