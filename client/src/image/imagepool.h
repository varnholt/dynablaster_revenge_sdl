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

   Image& getImage(const std::string& filename, int32_t flags = 0);

private:
   std::unordered_map<std::string, std::unique_ptr<Image>> _pool;
};
