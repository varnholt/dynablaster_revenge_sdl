#include "psd.h"

#include "tools/filestream.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <ranges>
#include <sstream>

// https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/

namespace
{
constexpr uint8_t visibility_flag = 0x02;

// four-character code as read big endian
constexpr int32_t fourCC(const char (&code)[5])
{
   return static_cast<int32_t>(
      (static_cast<uint32_t>(code[0]) << 24) | (static_cast<uint32_t>(code[1]) << 16) | (static_cast<uint32_t>(code[2]) << 8) |
      static_cast<uint32_t>(code[3])
   );
}

// photoshop files are big endian
template <typename T>
void read(T& value, std::istream& stream)
{
   std::array<char, sizeof(T)> bytes{};
   stream.read(bytes.data(), bytes.size());
   std::ranges::reverse(bytes);
   std::memcpy(&value, bytes.data(), sizeof(T));
}

template <size_t size>
void read(std::array<uint8_t, size>& value, std::istream& stream)
{
   stream.read(reinterpret_cast<char*>(value.data()), size);
}

void read(std::vector<uint8_t>& value, std::istream& stream)
{
   stream.read(reinterpret_cast<char*>(value.data()), static_cast<std::streamsize>(value.size()));
}
}  // namespace

// Header

int32_t PSD::Header::getWidth() const
{
   return _width;
}

int32_t PSD::Header::getHeight() const
{
   return _height;
}

void PSD::Header::load(std::istream& stream)
{
   read(_signature, stream);
   read(_version, stream);
   read(_reserved, stream);
   read(_channels, stream);
   read(_height, stream);
   read(_width, stream);
   read(_depth, stream);
   read(_mode, stream);
}

// Layer

const std::string& PSD::Layer::getName() const
{
   return _name;
}

void PSD::Layer::setName(const std::string& name)
{
   _name = name;
}

PSD::Layer::SectionDivider PSD::Layer::getSectionDivider() const
{
   return _section_divider;
}

bool PSD::Layer::isSectionDivider() const
{
   return _section_divider != SectionDivider::None && _section_divider != SectionDivider::Any;
}

bool PSD::Layer::isImageLayer() const
{
   return !isSectionDivider() && getWidth() > 0 && getHeight() > 0;
}

const std::string& PSD::Layer::getGroup() const
{
   return _group;
}

int32_t PSD::Layer::getBottom() const
{
   return _bottom;
}

int32_t PSD::Layer::getTop() const
{
   return _top;
}

int32_t PSD::Layer::getLeft() const
{
   return _left;
}

int32_t PSD::Layer::getRight() const
{
   return _right;
}

int32_t PSD::Layer::getWidth() const
{
   return _right - _left;
}

int32_t PSD::Layer::getHeight() const
{
   return _bottom - _top;
}

void PSD::Layer::move(int32_t x, int32_t y)
{
   _right += x;
   _left += x;

   _top += y;
   _bottom += y;
}

void PSD::Layer::setX(int32_t x)
{
   const int32_t width = _right - _left;

   _left = x;
   _right = x + width;
}

void PSD::Layer::setY(int32_t y)
{
   const int32_t height = _bottom - _top;

   _top = y;
   _bottom = y + height;
}

void PSD::Layer::setBottom(int32_t value)
{
   _bottom = value;
}

void PSD::Layer::setTop(int32_t value)
{
   _top = value;
}

const Image& PSD::Layer::getImage() const
{
   return _image;
}

int32_t PSD::Layer::getOpacity() const
{
   return _opacity;
}

void PSD::Layer::setOpacity(int32_t opacity)
{
   _opacity = static_cast<uint8_t>(opacity);
}

bool PSD::Layer::isVisible() const
{
   return (_flags & visibility_flag) == 0;
}

void PSD::Layer::setVisible(bool visible)
{
   if (visible)
   {
      _flags &= ~visibility_flag;
   }
   else
   {
      _flags |= visibility_flag;
   }
}

const PSD::Layer::Channel& PSD::Layer::getChannel(int16_t id) const
{
   const auto it = std::ranges::find(_channels, id, &Channel::getID);
   return it != _channels.end() ? *it : _channels.back();
}

void PSD::Layer::loadLayerRecords(std::istream& stream)
{
   read(_top, stream);
   read(_left, stream);
   read(_bottom, stream);
   read(_right, stream);

   read(_channel_count, stream);
   _channels.resize(_channel_count);
   for (auto& channel : _channels)
   {
      channel.load(stream);
   }

   read(_blend_mode_signature, stream);
   read(_blend_mode_key, stream);
   read(_opacity, stream);
   read(_clipping, stream);
   read(_flags, stream);
   stream.ignore(1);  // filler

   int32_t extra_data_length = 0;
   read(extra_data_length, stream);
   const auto extra_data_start = stream.tellg();

   // layer mask / adjustment layer data
   int32_t size = 0;
   read(size, stream);
   stream.ignore(size);

   // layer blending ranges data
   read(size, stream);
   stream.ignore(size);

   _name = loadString(stream);

   // tagged blocks
   int32_t block_header = 0;
   while (extra_data_length - (stream.tellg() - extra_data_start) > 4)
   {
      uint8_t byte = 0;
      read(byte, stream);
      block_header = (block_header << 8) | byte;

      if (block_header == fourCC("8BIM"))
      {
         int32_t block_id = 0;
         int32_t block_size = 0;
         read(block_id, stream);
         read(block_size, stream);
         const auto block_start = stream.tellg();

         if (block_id == fourCC("lsct") || block_id == fourCC("lsdk"))
         {
            int32_t section_divider = 0;
            read(section_divider, stream);
            _section_divider =
               (section_divider >= 0 && section_divider <= 3) ? static_cast<SectionDivider>(section_divider) : SectionDivider::None;
         }
         else if (block_id == fourCC("luni"))
         {
            // unicode layer name, only the low byte of each utf-16 character is kept
            uint32_t length = 0;
            read(length, stream);
            _name.assign(length, '\0');
            for (auto& c : _name)
            {
               uint16_t word = 0;
               read(word, stream);
               c = static_cast<char>(word & 255);
            }
         }

         stream.ignore(block_size - (stream.tellg() - block_start));
         block_header = 0;
      }
   }

   stream.ignore(extra_data_length - (stream.tellg() - extra_data_start));
}

void PSD::Layer::loadChannelImageData(std::istream& stream)
{
   const int32_t width = getWidth();
   const int32_t height = getHeight();

   for (auto& channel : _channels)
   {
      uint16_t compression = 0;
      read(compression, stream);

      switch (compression)
      {
         case 0:  // raw
            channel.loadRaw(width, height, stream);
            break;
         case 1:  // rle
            channel.loadRLE(width, height, stream);
            break;
         default:  // zip (2, 3) is not supported
            break;
      }
   }

   // rgb-only layers are opaque
   if (_channel_count == 3)
   {
      _channels.emplace_back().init(-1, width, height);
      _channel_count++;
   }

   _image = Image(width, height);
   for (int32_t y = 0; y < height; y++)
   {
      uint32_t* destination = _image.getScanline(y);

      const uint8_t* red = getChannel(0).getScanline(y);
      const uint8_t* green = getChannel(1).getScanline(y);
      const uint8_t* blue = getChannel(2).getScanline(y);
      const uint8_t* alpha = getChannel(-1).getScanline(y);

      for (int32_t x = 0; x < width; x++)
      {
         const uint8_t a = alpha[x];
         const uint8_t r = a > 0 ? red[x] : 0;
         const uint8_t g = a > 0 ? green[x] : 0;
         const uint8_t b = a > 0 ? blue[x] : 0;
         destination[x] = (a << 24) | (r << 16) | (g << 8) | b;
      }
   }

   _channels.clear();
   _channels.shrink_to_fit();
}

// Channel

int16_t PSD::Layer::Channel::getID() const
{
   return _id;
}

void PSD::Layer::Channel::load(std::istream& stream)
{
   read(_id, stream);
   read(_size, stream);
}

void PSD::Layer::Channel::loadRLE(int32_t width, int32_t height, std::istream& stream)
{
   std::vector<uint16_t> scanline_sizes(static_cast<size_t>(std::max(height, 0)));
   for (auto& scanline_size : scanline_sizes)
   {
      read(scanline_size, stream);
   }

   _width = width;
   _data.assign(static_cast<size_t>(width) * height, 0);

   // packbits: a non-negative control byte copies control + 1 bytes, a negative one repeats the
   // next byte 1 - control times
   size_t offset = 0;
   for (const auto scanline_size : scanline_sizes)
   {
      const size_t scanline_end = offset + static_cast<size_t>(width);
      int32_t remaining = scanline_size;
      while (remaining > 0)
      {
         int8_t control = 0;
         read(control, stream);
         remaining--;

         if (control >= 0)
         {
            for (int32_t i = 0; i <= control; i++)
            {
               uint8_t value = 0;
               read(value, stream);
               if (offset < scanline_end)
               {
                  _data[offset++] = value;
               }
            }
            remaining -= control + 1;
         }
         else if (control > -128)
         {
            uint8_t value = 0;
            read(value, stream);
            remaining--;
            for (int32_t i = 0; i < 1 - control; i++)
            {
               if (offset < scanline_end)
               {
                  _data[offset++] = value;
               }
            }
         }
      }
      offset = scanline_end;
   }
}

void PSD::Layer::Channel::loadRaw(int32_t width, int32_t height, std::istream& stream)
{
   _width = width;
   _data.resize(static_cast<size_t>(width) * height);
   read(_data, stream);
}

// fully opaque channel
void PSD::Layer::Channel::init(int16_t id, int32_t width, int32_t height)
{
   _id = id;
   _width = width;
   _data.assign(static_cast<size_t>(width) * height, 0xff);
}

const uint8_t* PSD::Layer::Channel::getScanline(int32_t y) const
{
   return _data.data() + static_cast<size_t>(y) * _width;
}

// PSD

int32_t PSD::getWidth() const
{
   return _header.getWidth();
}

int32_t PSD::getHeight() const
{
   return _header.getHeight();
}

size_t PSD::getLayerCount() const
{
   return _layers.size();
}

const std::vector<PSD::Layer>& PSD::getLayers() const
{
   return _layers;
}

std::vector<PSD::Layer>& PSD::getLayers()
{
   return _layers;
}

const PSD::Layer& PSD::getLayer(size_t index) const
{
   return _layers[index];
}

PSD::Layer& PSD::getLayer(size_t index)
{
   return _layers[index];
}

std::vector<PSD::Layer>::const_iterator PSD::getLayer(const std::string& name) const
{
   return std::ranges::find(_layers, name, &Layer::getName);
}

std::vector<PSD::Layer>::iterator PSD::getLayer(const std::string& name)
{
   return std::ranges::find(_layers, name, &Layer::getName);
}

void PSD::addLayer(Layer layer)
{
   _layers.push_back(std::move(layer));
}

// pascal string (leading length byte)
std::string PSD::loadString(std::istream& stream)
{
   uint8_t size = 0;
   read(size, stream);
   std::string text(size, '\0');
   stream.read(text.data(), size);
   return text;
}

void PSD::loadColorModeData(std::istream& stream)
{
   // only indexed color and duotone have color mode data
   int32_t length = 0;
   read(length, stream);
   stream.ignore(length);
}

void PSD::loadImageResourceSection(std::istream& stream)
{
   // resources are not used
   int32_t length = 0;
   read(length, stream);
   stream.ignore(length);
}

void PSD::loadLayerAndMaskInformation(std::istream& stream)
{
   // length of the layer and mask information section
   int32_t length = 0;
   read(length, stream);

   // length of the layers info section
   read(length, stream);

   // negative count: first alpha channel holds the merged transparency
   int16_t layer_count = 0;
   read(layer_count, stream);

   _layers.clear();
   _layers.resize(static_cast<size_t>(std::abs(layer_count)));
   for (auto& layer : _layers)
   {
      layer.loadLayerRecords(stream);
   }

   for (auto& layer : _layers)
   {
      layer.loadChannelImageData(stream);
   }

   assignGroups();
}

void PSD::assignGroups()
{
   // layers are stored bottom to top, so walking top down a folder layer opens its group and
   // the matching divider closes it again
   std::vector<std::string> groups;
   for (auto& layer : std::views::reverse(_layers))
   {
      switch (layer._section_divider)
      {
         case Layer::SectionDivider::OpenFolder:
         case Layer::SectionDivider::ClosedFolder:
            layer._group = groups.empty() ? std::string() : groups.back();
            groups.push_back(layer._name);
            break;
         case Layer::SectionDivider::BoundingSectionDivider:
            if (!groups.empty())
            {
               groups.pop_back();
            }
            layer._group = groups.empty() ? std::string() : groups.back();
            break;
         default:
            layer._group = groups.empty() ? std::string() : groups.back();
            break;
      }
   }
}

void PSD::load(std::istream& stream)
{
   _header.load(stream);
   loadColorModeData(stream);
   loadImageResourceSection(stream);
   loadLayerAndMaskInformation(stream);
}

bool PSD::load(const std::string& filename)
{
   FileStream file;
   if (!file.open(filename.c_str()))
   {
      return false;
   }

   std::string data(static_cast<size_t>(file.size()), '\0');
   file.getData(data.data(), static_cast<int32_t>(data.size()));

   std::istringstream stream(std::move(data));
   load(stream);
   return !stream.fail();
}
