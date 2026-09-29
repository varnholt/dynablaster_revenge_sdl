#include "imagepool.h"

#include <utility>

ImagePool::~ImagePool()
{
   // move out first: every deleted image calls back into remove() while being destroyed
   auto images = std::move(_pool);
   _pool.clear();
   images.clear();
}

void ImagePool::remove(Image* image)
{
   for (auto entry = _pool.begin(); entry != _pool.end();)
   {
      if (entry->second.get() == image)
      {
         [[maybe_unused]] Image* released = entry->second.release();
         entry = _pool.erase(entry);
      }
      else
      {
         ++entry;
      }
   }
}

Image* ImagePool::getImage(const char* filename, int32_t /*preprocessing_flags*/)
{
   const std::string name(filename);
   if (const auto entry = _pool.find(name); entry != _pool.end())
   {
      return entry->second.get();
   }

   auto image = std::make_unique<Image>(filename);
   Image* result = image.get();
   _pool[name] = std::move(image);
   return result;
}
