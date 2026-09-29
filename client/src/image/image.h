#pragma once

#include <cstdint>

#include "tools/referenced.h"
#include "tools/string.h"

// 32bit argb image; copies share the pixel buffer (see Referenced)
class Image : public Referenced
{
public:
   Image() = default;
   Image(const char* filename);
   Image(int32_t x, int32_t y);
   Image(const Image& image);
   ~Image() override;

   const Image& operator=(const Image& image);

   void discard();
   void clear(uint32_t argb);

   void save(const char* filename);

   int32_t getWidth() const;
   int32_t getHeight() const;
   uint32_t* getScanline(int32_t y) const;
   uint32_t* getData() const;
   Image downsample() const;
   void scaled(const Image& image) const;
   void load(const char* filename);
   void copy(int32_t x, int32_t y, const Image& source, int32_t clamp = 0);
   void premultiplyAlpha();
   void minimum(const Image& image);
   uint32_t getPixel(float u, float v) const;
   void buildNormalMap(int32_t z);
   void buildDeltaMap();
   const String& path() const;
   const String& filename() const;

private:
   // new[]-allocated, shared by all copies, deleted by the last one
   uint32_t* _data = nullptr;
   int32_t _width = 0;
   int32_t _height = 0;
   String _path;
   String _filename;
};
