// basic tga loader

#include "tga.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <span>
#include <string>

#include "tools/datapaths.h"
#include "tools/stream.h"

namespace
{
class TGAHeader
{
public:
   TGAHeader() = default;

   TGAHeader(uint16_t width, uint16_t height, uint8_t bits_per_pixel)
       : _image_type(2),  // rgb + no rle
         _width(width),
         _height(height),
         _bits_per_pixel(bits_per_pixel)
   {
   }

   void read(Stream& stream)
   {
      _ident_size = stream.getByte();  // number of ident bytes after header, usually 0
      _color_map_type = stream.getByte();
      _image_type = stream.getByte();
      _rle = _image_type >= 8;
      _image_type &= 7;

      _color_map_start = static_cast<int16_t>(stream.getWord());
      _color_map_length = static_cast<int16_t>(stream.getWord());
      _color_map_format = stream.getByte();

      _origin_x = static_cast<int16_t>(stream.getWord());
      _origin_y = static_cast<int16_t>(stream.getWord());

      _width = static_cast<uint16_t>(stream.getWord());
      _height = static_cast<uint16_t>(stream.getWord());
      _bits_per_pixel = stream.getByte();
      _descriptor = stream.getByte();

      // skip ident data
      stream.skip(_ident_size);
   }

   void write(Stream& stream)
   {
      stream.writeByte(_ident_size);
      stream.writeByte(_color_map_type);
      stream.writeByte(_image_type);

      stream.writeWord(_color_map_start);
      stream.writeWord(_color_map_length);
      stream.writeByte(_color_map_format);

      stream.writeWord(_origin_x);
      stream.writeWord(_origin_y);

      stream.writeWord(_width);
      stream.writeWord(_height);
      stream.writeByte(_bits_per_pixel);
      stream.writeByte(_descriptor);

      for (int32_t i = 0; i < _ident_size; i++)
      {
         stream.writeByte(0);
      }
   }

   uint8_t _ident_size = 0;      // size of ID field that follows 18 byte header (0 usually)
   uint8_t _color_map_type = 0;  // type of colour map 0=none, 1=has palette
   uint8_t _image_type = 0;      // type of image 0=none,1=indexed,2=rgb,3=grey,+8=rle packed

   int16_t _color_map_start = 0;   // first colour map entry in palette
   int16_t _color_map_length = 0;  // number of colours in palette
   uint8_t _color_map_format = 0;  // number of bits per palette entry 15,16,24,32

   int16_t _origin_x = 0;  // image x origin
   int16_t _origin_y = 0;  // image y origin

   uint16_t _width = 0;
   uint16_t _height = 0;
   uint8_t _bits_per_pixel = 0;
   uint8_t _descriptor = 0;
   bool _rle = false;
};

using Palette = std::array<uint32_t, 256>;

void loadPal24(Stream& stream, std::span<uint32_t> destination)
{
   for (uint32_t& entry : destination)
   {
      const uint8_t r = stream.getByte();
      const uint8_t g = stream.getByte();
      const uint8_t b = stream.getByte();
      entry = (255u << 24) + (b << 16) + (g << 8) + (r);
   }
}

void loadPal32(Stream& stream, std::span<uint32_t> destination)
{
   for (uint32_t& entry : destination)
   {
      const uint8_t r = stream.getByte();
      const uint8_t g = stream.getByte();
      const uint8_t b = stream.getByte();
      const uint8_t a = stream.getByte();
      entry = (a << 24) + (b << 16) + (g << 8) + (r);
   }
}

uint32_t loadPixel8(Stream& stream, const Palette& palette)
{
   const uint8_t index = stream.getByte();
   return palette[index];
}

uint32_t loadPixel16(Stream& stream, const Palette& /*palette*/)
{
   const auto rgb = static_cast<uint16_t>(stream.getWord());

   const uint8_t a = (rgb >> 15 & 1) * 255;
   const uint8_t r = (rgb & 31) << 3;
   const uint8_t g = (rgb >> 5 & 31) << 3;
   const uint8_t b = (rgb >> 10 & 31) << 3;

   return (a << 24) + (b << 16) + (g << 8) + (r);
}

uint32_t loadPixel24(Stream& stream, const Palette& /*palette*/)
{
   const uint8_t a = 255;
   const uint8_t r = stream.getByte();
   const uint8_t g = stream.getByte();
   const uint8_t b = stream.getByte();

   return (a << 24) + (b << 16) + (g << 8) + (r);
}

uint32_t loadPixel32(Stream& stream, const Palette& /*palette*/)
{
   const uint8_t r = stream.getByte();
   const uint8_t g = stream.getByte();
   const uint8_t b = stream.getByte();
   const uint8_t a = stream.getByte();

   return (a << 24) + (b << 16) + (g << 8) + (r);
}
}  // namespace

int32_t loadtga(const std::string& fname, std::vector<uint32_t>& pixels, int32_t& width, int32_t& height)
{
   TGAHeader info;
   Palette palette{};

   std::ifstream file = DataPaths::open(fname + ".tga");
   Stream stream(file);

   if (!file.is_open())
   {
      width = 1;
      height = 1;
      pixels.assign(1, 0xffffffff);
      return 0;
   }

   info.read(stream);

   pixels.assign(static_cast<size_t>(info._width) * info._height, 0);
   const std::span<uint32_t> data = pixels;

   if (info._image_type == 1)  // indexed colors
   {
      if (info._color_map_length <= 256)
      {
         switch (info._color_map_format)
         {
            case 24:
               loadPal24(stream, std::span(palette).first(std::max<int32_t>(0, info._color_map_length)));
               break;
            case 32:
               loadPal32(stream, std::span(palette).first(std::max<int32_t>(0, info._color_map_length)));
               break;
            default:
               break;
         }
      }
   }
   else if (info._image_type == 3)  // grey-scale
   {
      // create grey palette, so we can handle greyscale just as 8bit data
      for (uint32_t i = 0; i < palette.size(); i++)
      {
         palette[i] = (255u << 24) + (i << 16) + (i << 8) + (i);
      }
   }

   const bool top_down = (info._descriptor >> 5 & 1) != 0;
   std::function<uint32_t(Stream&, const Palette&)> loadPixel;
   switch (info._bits_per_pixel)
   {
      case 8:
         loadPixel = loadPixel8;
         break;
      case 16:
         loadPixel = loadPixel16;
         break;
      case 24:
         loadPixel = loadPixel24;
         break;
      case 32:
         loadPixel = loadPixel32;
         break;
      default:
         break;
   }

   const auto scanline = [&](int32_t y)
   { return data.subspan(static_cast<size_t>(top_down ? y : info._height - 1 - y) * info._width, info._width); };

   if (info._rle)
   {
      int32_t scan = 0;
      int32_t count = 0;
      bool single = false;
      uint32_t color = 0;
      std::span<uint32_t> destination;
      int32_t y = 0;
      while (true)
      {
         // next scanline?
         if (scan == 0)
         {
            scan = info._width;
            y++;
            if (y > info._height)
            {
               break;
            }
            destination = scanline(y - 1);
         }

         if (count == 0)
         {
            count = stream.getByte() + 1;
            if (count > 128)
            {
               // repeat single pixel color
               single = true;
               count -= 128;
               color = loadPixel(stream, palette);
            }
            else
            {
               single = false;
            }
         }

         const int32_t length = scan < count ? scan : count;
         const int32_t offset = info._width - scan;
         for (int32_t x = 0; x < length; x++)
         {
            destination[offset + x] = single ? color : loadPixel(stream, palette);
         }

         count -= length;
         scan -= length;
      }
   }
   else
   {
      for (int32_t y = 0; y < info._height; y++)
      {
         const std::span<uint32_t> destination = scanline(y);
         for (int32_t x = 0; x < info._width; x++)
         {
            destination[x] = loadPixel(stream, palette);
         }
      }
   }

   width = info._width;
   height = info._height;

   return 32;
}

int32_t savetga(const std::string& fname, std::span<const uint32_t> data, int32_t width, int32_t height)
{
   std::ofstream file(fname, std::ios::binary);
   Stream stream(file);
   TGAHeader info(static_cast<uint16_t>(width), static_cast<uint16_t>(height), 32);

   if (!file.is_open())
   {
      return 0;
   }

   info.write(stream);
   for (int32_t y = 0; y < height; y++)
   {
      const std::span<const uint32_t> source = data.subspan(static_cast<size_t>(height - 1 - y) * info._width, info._width);
      for (int32_t x = 0; x < info._width; x++)
      {
         const uint32_t c = source[x];
         const uint8_t b = c & 255;
         const uint8_t g = c >> 8 & 255;
         const uint8_t r = c >> 16 & 255;
         const uint8_t a = c >> 24 & 255;

         stream.writeByte(b);
         stream.writeByte(g);
         stream.writeByte(r);
         stream.writeByte(a);
      }
   }

   file.close();

   return 32;
}
