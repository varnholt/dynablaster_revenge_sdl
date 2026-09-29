#pragma once

#include "image.h"
#include "tools/singleton.h"

#include <memory>
#include <string>
#include <unordered_map>

// owns every image it loaded
class ImagePool : public Singleton<ImagePool>
{
public:
   ImagePool() = default;
   ~ImagePool() override;

   Image* getImage(const char* filename, int32_t flags = 0);

   // releases ownership of "image" without deleting it (called by Image::discard())
   void remove(Image* image);

private:
   std::unordered_map<std::string, std::unique_ptr<Image>> _pool;
};
