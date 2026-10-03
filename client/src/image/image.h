#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

// 32bit argb image; copies share the pixel buffer
class Image
{
public:
   Image() = default;
   explicit Image(const std::string& filename);
   Image(int32_t x, int32_t y);

   void clear(uint32_t argb);

   void save(const std::string& filename);

   int32_t getWidth() const;
   int32_t getHeight() const;
   std::span<uint32_t> getScanline(int32_t y) const;
   std::span<uint32_t> getData() const;
   Image downsample() const;
   void scaled(const Image& image) const;
   void load(const std::string& filename);
   void copy(int32_t x, int32_t y, const Image& source, int32_t clamp = 0);
   void premultiplyAlpha();
   void minimum(const Image& image);
   uint32_t getPixel(float u, float v) const;
   void buildNormalMap(int32_t z);
   void buildDeltaMap();
   const std::string& path() const;
   const std::string& filename() const;

private:
   std::shared_ptr<std::vector<uint32_t>> _pixels;
   int32_t _width = 0;
   int32_t _height = 0;
   std::string _path;
   std::string _filename;
};
