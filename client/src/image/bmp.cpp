#include "bmp.h"

#include <array>
#include <fstream>

#include "tools/stream.h"

namespace
{
#pragma pack(push, 1)

struct BMPInfoHeader
{
   BMPInfoHeader(int32_t x, int32_t y) : _width(x), _height(y), _image_size(x * y * 3)
   {
   }

   uint32_t _size = 40;  // sizeof(BMPInfoHeader)
   int32_t _width = 0;
   int32_t _height = 0;
   uint16_t _planes = 1;
   uint16_t _bit_count = 24;
   uint32_t _compression = 0;
   uint32_t _image_size = 0;
   int32_t _x_pels_per_meter = 0;
   int32_t _y_pels_per_meter = 0;
   uint32_t _colors_used = 0;
   uint32_t _colors_important = 0;
};

struct BMPFileHeader
{
   std::array<char, 2> _head = {'B', 'M'};
   uint32_t _size = 0;
   uint32_t _reserved = 0;
   uint32_t _offset = 14 + 40;  // sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)
};

#pragma pack(pop)

static_assert(sizeof(BMPInfoHeader) == 40);
static_assert(sizeof(BMPFileHeader) == 14);
}  // namespace

int32_t saveBmp(const std::string& filename, std::span<const uint32_t> data, int32_t width, int32_t height)
{
   std::ofstream file(filename, std::ios::binary);
   Stream stream(file);

   BMPFileHeader head;
   stream.writeRaw(head);

   BMPInfoHeader info(width, height);
   stream.writeRaw(info);

   const int32_t padding = (4 - width * 3) & 3;

   for (int32_t y = 0; y < height; y++)
   {
      const std::span<const uint32_t> source = data.subspan(static_cast<size_t>(y) * width, width);
      for (int32_t x = 0; x < width; x++)
      {
         const uint32_t color = source[x];

         const uint8_t r = color >> 16 & 255;
         const uint8_t g = color >> 8 & 255;
         const uint8_t b = color & 255;

         stream.writeByte(b);
         stream.writeByte(g);
         stream.writeByte(r);
      }

      for (int32_t p = 0; p < padding; p++)
      {
         stream.writeByte(0);
      }
   }

   file.close();

   return 1;
}
