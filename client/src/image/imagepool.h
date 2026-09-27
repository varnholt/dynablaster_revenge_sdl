#ifndef IMAGEPOOL_H
#define IMAGEPOOL_H

#include "image.h"
#include "tools/singleton.h"

#include <string>
#include <unordered_map>

class ImagePool : public Singleton<ImagePool>
{
public:
   ImagePool();
   ~ImagePool();

   Image* getImage(const char* filename, int flags = 0);

   void remove(Image* image);

private:
   std::unordered_map<std::string, Image*> mPool;
};

#endif
