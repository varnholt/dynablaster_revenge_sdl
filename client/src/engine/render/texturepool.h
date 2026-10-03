#pragma once

#include "texture.h"
#include "tools/singleton.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class Image;

class TexturePool : public Singleton<TexturePool>
{
public:
   // bit flags
   enum Filter
   {
      Nearest = 0,
      Linear = 1,
      MipMap = 2,
      Trilinear = Linear | MipMap,
      Clamp = 4,
      PremultipliedAlpha = 8
   };

   TexturePool() = default;

   Texture getTexture(const std::string& filename, int32_t filter = Trilinear);
   Texture getTexture(const Image& image, int32_t filter = Trilinear);

   void remove(const Texture& texture);

   void update();

private:
   std::unordered_map<std::string, Texture> _pool;
   std::vector<uint32_t> _removal;
   bool _block = false;
   uint32_t _memory = 0;
};
