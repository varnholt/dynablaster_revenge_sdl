#include "image.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include "tga.h"
#include "tools/filestream.h"

// construct empty (black, transparent) image x*y
Image::Image(int32_t x, int32_t y) : _pixels(std::make_shared<std::vector<uint32_t>>(static_cast<size_t>(x) * y)), _width(x), _height(y)
{
}

// construct image from file
Image::Image(const char* filename)
{
   load(filename);
}

void Image::save(const char* filename)
{
   savetga(filename, getData(), getWidth(), getHeight());
}

void Image::load(const char* filename)
{
   // copies keep the previous pixels
   auto pixels = std::make_shared<std::vector<uint32_t>>();
   const bool loaded = loadtga(filename, *pixels, _width, _height) != 0;
   _pixels = std::move(pixels);

   if (loaded)
   {
      const std::string name = std::string(filename) + ".tga";
      FileStream stream;
      if (stream.open(name))
      {
         _path = stream.getPath();
         _filename = filename;
         stream.close();
      }
   }
}

int32_t Image::getWidth() const
{
   return _width;
}

int32_t Image::getHeight() const
{
   return _height;
}

uint32_t* Image::getScanline(int32_t y) const
{
   return getData() + y * _width;
}

uint32_t* Image::getData() const
{
   return (_pixels && !_pixels->empty()) ? _pixels->data() : nullptr;
}

const std::string& Image::path() const
{
   return _path;
}

const std::string& Image::filename() const
{
   return _filename;
}

uint32_t Image::getPixel(float u, float v) const
{
   const auto x = static_cast<int32_t>(std::floor(u * (_width - 1)));
   const auto y = static_cast<int32_t>(std::floor(v * (_height - 1)));
   return getData()[y * _width + x];
}

// halve resolution
Image Image::downsample() const
{
   const int32_t next_width = _width >> 1;
   const int32_t next_height = _height >> 1;

   Image image(next_width, next_height);

   for (int32_t y = 0; y < next_height; y++)
   {
      uint32_t* destination = image.getScanline(y);

      const uint32_t* source1 = getScanline(y * 2);
      const uint32_t* source2 = getScanline(y * 2 + 1);

      for (int32_t x = 0; x < next_width; x++)
      {
         const uint32_t c1 = *source1++;
         const uint32_t c2 = *source1++;
         const uint32_t c3 = *source2++;
         const uint32_t c4 = *source2++;

         const uint32_t a = ((c1 >> 24 & 0xff) + (c2 >> 24 & 0xff) + (c3 >> 24 & 0xff) + (c4 >> 24 & 0xff)) >> 2;
         const uint32_t r = ((c1 >> 16 & 0xff) + (c2 >> 16 & 0xff) + (c3 >> 16 & 0xff) + (c4 >> 16 & 0xff)) >> 2;
         const uint32_t g = ((c1 >> 8 & 0xff) + (c2 >> 8 & 0xff) + (c3 >> 8 & 0xff) + (c4 >> 8 & 0xff)) >> 2;
         const uint32_t b = ((c1 & 0xff) + (c2 & 0xff) + (c3 & 0xff) + (c4 & 0xff)) >> 2;

         *destination++ = (a << 24) + (r << 16) + (g << 8) + b;
      }
   }

   return image;
}

namespace
{
// linear blend between c1 & c2
uint32_t blend(uint32_t c1, uint32_t c2, uint8_t f)
{
   const uint8_t a = (c1 >> 24 & 0xff) + (((c2 >> 24 & 0xff) - (c1 >> 24 & 0xff)) * f >> 8);
   const uint8_t r = (c1 >> 16 & 0xff) + (((c2 >> 16 & 0xff) - (c1 >> 16 & 0xff)) * f >> 8);
   const uint8_t g = (c1 >> 8 & 0xff) + (((c2 >> 8 & 0xff) - (c1 >> 8 & 0xff)) * f >> 8);
   const uint8_t b = (c1 & 0xff) + (((c2 & 0xff) - (c1 & 0xff)) * f >> 8);

   return (a << 24) | (r << 16) | (g << 8) | b;
}

uint32_t calcNormal(int32_t z, uint32_t x0, uint32_t x1, uint32_t y0, uint32_t y1)
{
   // height= red + green + blue;
   x0 = (x0 >> 16 & 255) + (x0 >> 8 & 255) + (x0 & 255);
   x1 = (x1 >> 16 & 255) + (x1 >> 8 & 255) + (x1 & 255);
   y0 = (y0 >> 16 & 255) + (y0 >> 8 & 255) + (y0 & 255);
   y1 = (y1 >> 16 & 255) + (y1 >> 8 & 255) + (y1 & 255);

   int32_t x = static_cast<int32_t>(x0 - x1);
   int32_t y = static_cast<int32_t>(y0 - y1);

   const int32_t magnitude = x * x + y * y + z * z;
   const auto t = static_cast<float>(128.0f / std::sqrt(static_cast<double>(magnitude)));
   x = std::clamp(static_cast<int32_t>(128 + x * t), 0, 255);
   y = std::clamp(static_cast<int32_t>(128 - y * t), 0, 255);
   z = std::clamp(static_cast<int32_t>(128 + z * t), 0, 255);

   return (255u << 24) | (x << 16) | (y << 8) | z;
}
}  // namespace

// fill this image with a bilinearly scaled version of "image"
void Image::scaled(const Image& image) const
{
   const int32_t width = image.getWidth();
   const int32_t height = image.getHeight();

   const int32_t dx = (width << 16) / _width;
   const int32_t dy = (height << 16) / _height;

   int32_t iy = 0;
   for (int32_t destination_y = 0; destination_y < _height; destination_y++)
   {
      const int32_t y = iy >> 16;
      const auto sy = static_cast<uint8_t>(iy >> 8 & 0xff);

      uint32_t* destination = getScanline(destination_y);
      const uint32_t* source1 = image.getScanline(y);
      // do not exceed image boundaries
      const uint32_t* source2 = (y == height - 1) ? image.getScanline(y) : image.getScanline(y + 1);

      int32_t ix = 0;
      for (int32_t destination_x = 0; destination_x < _width - 1; destination_x++)
      {
         const int32_t x = ix >> 16;
         const auto sx = static_cast<uint8_t>(ix >> 8 & 0xff);

         const uint32_t top = blend(source1[x], source1[x + 1], sx);
         const uint32_t bottom = blend(source2[x], source2[x + 1], sx);

         destination[destination_x] = blend(top, bottom, sy);

         ix += dx;
      }
      destination[_width - 1] = blend(source1[width - 1], source2[width - 1], sy);

      iy += dy;
   }
}

void Image::premultiplyAlpha()
{
   for (int32_t y = 0; y < _height; y++)
   {
      uint32_t* destination = getScanline(y);

      for (int32_t x = 0; x < _width; x++)
      {
         const uint32_t c1 = destination[x];

         const uint8_t a = (c1 >> 24 & 0xff);
         if (a != 255)
         {
            uint8_t r = (c1 >> 16 & 0xff);
            uint8_t g = (c1 >> 8 & 0xff);
            uint8_t b = (c1 & 0xff);

            r = (r * a) >> 8;
            g = (g * a) >> 8;
            b = (b * a) >> 8;

            destination[x] = (a << 24) + (r << 16) + (g << 8) + b;
         }
      }
   }
}

// per-channel minimum with "image"
void Image::minimum(const Image& image)
{
   const int32_t width = std::min(_width, image.getWidth());
   const int32_t height = std::min(_height, image.getHeight());

   for (int32_t y = 0; y < height; y++)
   {
      uint32_t* destination = getScanline(y);
      const uint32_t* source = image.getScanline(y);

      for (int32_t x = 0; x < width; x++)
      {
         const uint32_t c1 = source[x];
         const uint32_t c2 = destination[x];

         const uint32_t a = std::min(c1 >> 24 & 0xff, c2 >> 24 & 0xff);
         const uint32_t r = std::min(c1 >> 16 & 0xff, c2 >> 16 & 0xff);
         const uint32_t g = std::min(c1 >> 8 & 0xff, c2 >> 8 & 0xff);
         const uint32_t b = std::min(c1 & 0xff, c2 & 0xff);

         destination[x] = (a << 24) + (r << 16) + (g << 8) + b;
      }
   }
}

void Image::clear(uint32_t argb)
{
   for (int32_t y = 0; y < _height; y++)
   {
      std::fill_n(getScanline(y), _width, argb);
   }
}

void Image::copy(int32_t position_x, int32_t position_y, const Image& image, int32_t replicate)
{
   const int32_t width = std::min(_width - position_x, image.getWidth());
   const int32_t height = std::min(_height - position_y, image.getHeight());

   const int32_t end_x = _width - position_x;
   const int32_t end_y = _height - position_y;

   for (int32_t y = 0; y < height; y++)
   {
      uint32_t* destination = getScanline(y + position_y) + position_x;
      const uint32_t* source = image.getScanline(y);

      for (int32_t x = 0; x < width; x++)
      {
         destination[x] = source[x];
      }

      // replicate last pixel
      for (int32_t x = 0; x < end_x - width && x < replicate; x++)
      {
         destination[width + x] = source[width - 1];
      }
   }

   // replicate last scanline
   if (replicate > 0)
   {
      const uint32_t* source = getScanline(height - 1);
      for (int32_t y = 0; y < end_y - height && y < replicate; y++)
      {
         uint32_t* destination = getScanline(y + height) + position_x;
         std::memcpy(destination, source, end_x * sizeof(uint32_t));
      }
   }
}

void Image::buildNormalMap(int32_t z)
{
   const auto source_pixels = _pixels;
   auto target_pixels = std::make_shared<std::vector<uint32_t>>(static_cast<size_t>(_width) * _height);
   const uint32_t* source = source_pixels->data();
   const uint32_t* source0 = source + (_height - 1) * _width;
   const uint32_t* source1 = source;
   const uint32_t* source2 = source + _width;
   uint32_t* destination = target_pixels->data();

   for (int32_t y = 0; y < _height; y++)
   {
      destination[0] = calcNormal(z, source1[_width - 1], source1[1], source0[0], source2[0]);
      for (int32_t x = 1; x < _width - 1; x++)
      {
         destination[x] = calcNormal(z, source1[x - 1], source1[x + 1], source0[x], source2[x]);
      }
      destination[_width - 1] = calcNormal(z, source1[_width - 2], source1[0], source0[_width - 1], source2[_width - 1]);

      destination += _width;
      source0 = source1;
      source1 = source2;
      // wrap around to the first source row
      source2 = (y < _height - 2) ? source2 + _width : source;
   }

   _pixels = std::move(target_pixels);
}

void Image::buildDeltaMap()
{
   const auto source_pixels = _pixels;
   auto target_pixels = std::make_shared<std::vector<uint32_t>>(static_cast<size_t>(_width) * _height);
   const uint32_t* source = source_pixels->data();
   const uint32_t* source0 = source + (_height - 1) * _width;
   const uint32_t* source1 = source;
   const uint32_t* source2 = source + _width;
   uint32_t* destination = target_pixels->data();
   const uint32_t s = 2;

   const auto delta = [s](uint32_t left, uint32_t right, uint32_t up, uint32_t down)
   { return ((128 + (left & 0xff) * s - (right & 0xff) * s) << 16) | ((128 + (up & 0xff) * s - (down & 0xff) * s) << 8); };

   for (int32_t y = 0; y < _height; y++)
   {
      destination[0] = delta(source1[_width - 1], source1[1], source0[0], source2[0]);
      for (int32_t x = 1; x < _width - 1; x++)
      {
         destination[x] = delta(source1[x - 1], source1[x + 1], source0[x], source2[x]);
      }
      destination[_width - 1] = delta(source1[_width - 2], source1[0], source0[_width - 1], source2[_width - 1]);

      destination += _width;
      source0 = source1;
      source1 = source2;
      // wrap around to the first source row
      source2 = (y < _height - 2) ? source2 + _width : source;
   }

   _pixels = std::move(target_pixels);
}
