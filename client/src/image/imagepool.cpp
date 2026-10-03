#include "imagepool.h"

#include <memory>

Image& ImagePool::getImage(const std::string& filename, int32_t /*preprocessing_flags*/)
{
   if (const auto entry = _pool.find(filename); entry != _pool.end())
   {
      return *entry->second;
   }

   return *(_pool[filename] = std::make_unique<Image>(filename));
}
