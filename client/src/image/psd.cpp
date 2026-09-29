#include "psd.h"

#include <algorithm>
#include <cstdlib>
#include <string_view>

#include "image/image.h"
#include "tools/filestream.h"

namespace
{
constexpr uint8_t kVisibilityFlag = 0x02;

// four-character code as read big endian by Stream::getInt()
constexpr uint32_t fourCC(const char (&code)[5])
{
   return (static_cast<uint32_t>(code[0]) << 24) | (static_cast<uint32_t>(code[1]) << 16) | (static_cast<uint32_t>(code[2]) << 8) |
          static_cast<uint32_t>(code[3]);
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

void PSD::Header::load(Stream* stream)
{
   stream->getData(_sign.data(), 4);
   _version = static_cast<uint16_t>(stream->getWord());
   stream->getData(_reserved.data(), 6);
   _channels = static_cast<uint16_t>(stream->getWord());
   _height = stream->getInt();
   _width = stream->getInt();
   _depth = static_cast<uint16_t>(stream->getWord());
   _mode = static_cast<ColorMode>(stream->getWord());
}

// Layer

PSD::Layer::Layer() = default;

PSD::Layer::~Layer() = default;

const char* PSD::Layer::getName() const
{
   return _name.c_str();
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

Image* PSD::Layer::getImage() const
{
   return _image.get();
}

int32_t PSD::Layer::getOpacity() const
{
   return _opacity;
}

bool PSD::Layer::isVisible() const
{
   return ((_flags & kVisibilityFlag) != 2);
}

void PSD::Layer::setVisible(bool visible)
{
   if (visible)
   {
      _flags &= ~kVisibilityFlag;
   }
   else
   {
      _flags |= kVisibilityFlag;
   }
}

const PSD::Layer::Channel* PSD::Layer::getChannel(int32_t id) const
{
   for (int32_t i = 0; i < _channel_count && i < static_cast<int32_t>(_channels.size()); i++)
   {
      if (_channels[i].getID() == id)
      {
         return &_channels[i];
      }
   }

   return nullptr;
}

void PSD::Layer::setOpacity(int32_t opacity)
{
   _opacity = static_cast<uint8_t>(opacity);
}

// load layer parameters from stream
void PSD::Layer::load(Stream* stream)
{
   std::array<char, 4> signature{};

   _top = stream->getInt();
   _left = stream->getInt();
   _bottom = stream->getInt();
   _right = stream->getInt();

   _channel_count = static_cast<uint16_t>(stream->getWord());
   // one spare slot for the opaque alpha channel loadChannels() adds to rgb-only layers
   _channels.resize(std::max<size_t>(4, _channel_count));

   for (int32_t i = 0; i < _channel_count; i++)
   {
      _channels[i].load(stream);
   }

   stream->getData(signature.data(), 4);
   // TODO: drop out if signature doesn't fit
   stream->getData(_blend_mode.data(), 4);
   _opacity = stream->getByte();
   _clipping = stream->getByte();
   _flags = stream->getByte();
   stream->getByte();  // filler

   const int32_t total_size = stream->getInt();
   const int32_t start_position = stream->pos();

   // Layer mask / adjust layer data
   int32_t size = stream->getInt();
   stream->skip(size);

   // Layer blending ranges data
   size = stream->getInt();
   stream->skip(size);

   _name = PSD::loadString(stream);

   uint32_t block_header = 0;
   while (total_size - (stream->pos() - start_position) > 4)
   {
      block_header = (block_header << 8) | stream->getByte();
      if (block_header == fourCC("8BIM"))
      {
         const auto block_id = static_cast<uint32_t>(stream->getInt());
         const int32_t block_size = stream->getInt();
         const int32_t block_position = stream->pos();
         if (block_id == fourCC("luni"))
         {
            // unicode layer name, only the low byte of each utf-16 character is kept
            const auto length = static_cast<uint32_t>(stream->getInt());
            _name.assign(length, '\0');
            for (uint32_t i = 0; i < length; i++)
            {
               _name[i] = static_cast<char>(stream->getWord() & 255);
            }
         }
         // skip rest of block
         stream->skip(block_size - (stream->pos() - block_position));
         block_header = 0;
      }
   }

   // skip rest of data
   stream->skip(total_size - (stream->pos() - start_position));
}

void PSD::Layer::loadChannels(Stream* stream)
{
   const int32_t height = _bottom - _top;
   const int32_t width = _right - _left;

   for (int32_t i = 0; i < _channel_count; i++)
   {
      const auto compression = static_cast<uint16_t>(stream->getWord());

      switch (compression)
      {
         case 0:  // Raw
            _channels[i].loadRaw(width, height, stream);
            break;

         case 1:  // RLE
            _channels[i].loadRLE(width, height, stream);
            break;

         case 2:  // Zip
            break;

         case 3:  // Zip Prediction
            break;

         default:
            break;
      }
   }

   if (_channel_count == 3)
   {
      _channels[_channel_count].init(-1, width, height);
      _channel_count++;
   }

   _image = std::make_unique<Image>(width, height);
   for (int32_t y = 0; y < height; y++)
   {
      uint32_t* destination = _image->getScanline(y);

      const uint8_t* red = getChannel(0)->getScanline(y);
      const uint8_t* green = getChannel(1)->getScanline(y);
      const uint8_t* blue = getChannel(2)->getScanline(y);
      const uint8_t* alpha = getChannel(-1)->getScanline(y);

      for (int32_t x = 0; x < width; x++)
      {
         const uint8_t a = alpha[x];
         uint8_t r = 0;
         uint8_t g = 0;
         uint8_t b = 0;
         if (a > 0)
         {
            r = red[x];
            g = green[x];
            b = blue[x];
         }
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

const uint8_t* PSD::Layer::Channel::data() const
{
   return _data.data();
}

void PSD::Layer::Channel::load(Stream* stream)
{
   _id = static_cast<int16_t>(stream->getWord());
   _size = stream->getInt();
}

void PSD::Layer::Channel::loadRLE(int32_t width, int32_t height, Stream* stream)
{
   _width = width;
   std::vector<uint16_t> scan_sizes(height);
   for (auto& scan_size : scan_sizes)
   {
      scan_size = static_cast<uint16_t>(stream->getWord());
   }

   _data.resize(static_cast<size_t>(width) * height);

   for (int32_t y = 0; y < height; y++)
   {
      uint8_t* destination = _data.data() + static_cast<size_t>(y) * width;

      int32_t size = scan_sizes[y];

      while (size > 0)
      {
         const int32_t start = stream->pos();

         const auto control = static_cast<int8_t>(stream->getChar());
         if (control >= 0)
         {
            stream->getData(destination, control + 1);
            destination += control + 1;
         }
         else if (control > -128)
         {
            const uint8_t color = stream->getByte();
            for (int32_t x = 0; x < (1 - control); x++)
            {
               *destination++ = color;
            }
         }

         const int32_t end = stream->pos();
         size -= (end - start);
      }
   }
}

void PSD::Layer::Channel::loadRaw(int32_t width, int32_t height, Stream* stream)
{
   _width = width;
   _data.resize(static_cast<size_t>(width) * height);

   for (int32_t y = 0; y < height; y++)
   {
      stream->getData(_data.data() + static_cast<size_t>(y) * width, width);
   }
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

PSD::PSD() = default;

PSD::~PSD() = default;

int32_t PSD::getWidth() const
{
   return _header.getWidth();
}

int32_t PSD::getHeight() const
{
   return _header.getHeight();
}

int32_t PSD::getLayerCount() const
{
   return _layer_count;
}

PSD::Layer* PSD::getLayer(int32_t index) const
{
   return &_layers[index];
}

PSD::Layer* PSD::getLayer(const char* name) const
{
   for (int32_t i = 0; i < _layer_count; i++)
   {
      if (std::string_view(_layers[i].getName()) == name)
      {
         return &_layers[i];
      }
   }
   return nullptr;
}

// load pascal string (leading length byte)
std::string PSD::loadString(Stream* stream)
{
   const uint8_t size = stream->getByte();
   std::string name(size, '\0');
   for (auto& c : name)
   {
      c = stream->getChar();
   }
   return name;
}

void PSD::loadImageResourceSection(Stream* stream)
{
   // resources are not used, skip the whole section
   const int32_t total = stream->getInt();
   stream->skip(total);
}

void PSD::loadLayerInformation(Stream* stream)
{
   // total size of layer and mask block
   stream->getInt();

   // size of layer block
   stream->getInt();

   // negative count: first alpha channel holds the merged transparency
   _layer_count = std::abs(stream->getShort());
   _layers = std::make_unique<Layer[]>(_layer_count);

   // load layer parameters
   for (int32_t i = 0; i < _layer_count; i++)
   {
      _layers[i].load(stream);
   }

   // load layer channels (bitmap data)
   for (int32_t i = 0; i < _layer_count; i++)
   {
      _layers[i].loadChannels(stream);
   }

   // TODO: skip mask block
}

bool PSD::load(const char* filename)
{
   FileStream stream;
   if (stream.open(filename))
   {
      return load(&stream);
   }
   return false;
}

bool PSD::load(Stream* stream)
{
   const int32_t endian = stream->getEndian();
   stream->setEndian(1);  // photoshop files are big endian

   _header.load(stream);

   // skip color mode data
   const int32_t size = stream->getInt();
   stream->skip(size);

   loadImageResourceSection(stream);

   loadLayerInformation(stream);

   stream->setEndian(endian);

   return true;
}
