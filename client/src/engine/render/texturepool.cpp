#include "texturepool.h"
#include "../materials/material.h"
#include "framework/renderdevice.h"
#include "image/image.h"

Texture TexturePool::getTexture(Image* image, int32_t flags)
{
   const uint32_t texture_id = Material::uploadMap(*image, flags);

   // keep track of consumed memory
   const int32_t size = image->getWidth() * image->getHeight();
   _memory += size * 4;
   if (flags & MipMap)  // approx. mipmaps
   {
      _memory += size * 4 / 3;
   }

   return Texture(texture_id);
}

Texture TexturePool::getTexture(const char* filename, int32_t flags)
{
   if (!filename)
   {
      return Texture();
   }

   const auto iterator = _pool.find(filename);
   if (iterator != _pool.end())
   {
      return iterator->second;
   }

   Image image(filename);
   Texture texture = getTexture(&image, flags);
   _pool[std::string(filename)] = texture;
   return texture;
}

void TexturePool::update()
{
   while (_removal.size() > 0)
   {
      const uint32_t texture_id = _removal.takeLast();
      activeDevice->deleteTexture(texture_id);
   }
}

void TexturePool::remove(const Texture& texture)
{
   if (_block)
   {
      return;
   }

   _block = true;
   bool found = false;

   // remove from pool
   for (auto iterator = _pool.begin(); iterator != _pool.end();)
   {
      const Texture& pooled = iterator->second;
      if (pooled.getTexture() == texture.getTexture() && texture.getRefCount() <= 2)
      {
         iterator = _pool.erase(iterator);
         found = true;
      }
      else
      {
         ++iterator;
      }
   }

   if (found || texture.getRefCount() == 1)
   {
      _removal.add(texture.getTexture());
   }

   _block = false;
}
