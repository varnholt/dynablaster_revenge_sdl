#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class Stream;
class Image;

/*
Header
Color Mode Data Block
Image Resource Block
Layer and Mask Information Block
Image Data
*/

class PSD
{
public:
   class Header
   {
   public:
      enum class ColorMode : uint16_t
      {
         Bitmap = 0,
         Grayscale = 1,
         Indexed = 2,
         RGB = 3,
         CMYK = 4,
         Multichannel = 7,
         Duotone = 8,
         Lab = 9
      };

      void load(Stream* stream);
      int32_t getWidth() const;
      int32_t getHeight() const;

   private:
      std::array<uint8_t, 4> _sign{};      // File ID "8BPS"
      uint16_t _version = 0;               // Version number, always 1
      std::array<uint8_t, 6> _reserved{};  // Reserved, must be zeroed
      uint16_t _channels = 0;              // Number of color channels (1-24) including alpha channels
      int32_t _height = 0;                 // Height of image in pixels (1-30000)
      int32_t _width = 0;                  // Width of image in pixels (1-30000)
      uint16_t _depth = 0;                 // Number of bits per channel (1, 8 and 16)
      ColorMode _mode = ColorMode::Bitmap;
   };

   class Layer
   {
   public:
      class Channel
      {
      public:
         void load(Stream* stream);
         void init(int16_t id, int32_t width, int32_t height);
         void loadRLE(int32_t width, int32_t height, Stream* stream);
         void loadRaw(int32_t width, int32_t height, Stream* stream);
         const uint8_t* getScanline(int32_t y) const;

         int16_t getID() const;
         const uint8_t* data() const;

      private:
         int16_t _id = 0;
         int32_t _size = 0;
         int32_t _width = 0;
         std::vector<uint8_t> _data;
      };

      Layer();
      ~Layer();

      void load(Stream* stream);
      void loadChannels(Stream* stream);

      int32_t getBottom() const;
      int32_t getTop() const;
      int32_t getLeft() const;
      int32_t getWidth() const;
      int32_t getHeight() const;
      void setOpacity(int32_t opacity);
      int32_t getOpacity() const;
      bool isVisible() const;
      void setVisible(bool visible);
      const char* getName() const;
      void move(int32_t x, int32_t y);
      void setX(int32_t x);
      void setY(int32_t y);
      void setBottom(int32_t value);
      void setTop(int32_t value);
      Image* getImage() const;

   private:
      const Channel* getChannel(int32_t id) const;

      int32_t _top = 0;
      int32_t _left = 0;
      int32_t _bottom = 0;
      int32_t _right = 0;
      uint16_t _channel_count = 0;
      std::unique_ptr<Image> _image;
      std::vector<Channel> _channels;  // only valid while loading
      std::array<char, 4> _blend_mode{};
      uint8_t _opacity = 0;
      uint8_t _clipping = 0;
      uint8_t _flags = 0;
      std::string _name;
   };

   PSD();
   ~PSD();

   int32_t getWidth() const;
   int32_t getHeight() const;
   int32_t getLayerCount() const;
   Layer* getLayer(int32_t index) const;
   Layer* getLayer(const char* name) const;

   bool load(const char* filename);
   bool load(Stream* stream);

   static std::string loadString(Stream* stream);

private:
   void loadImageResourceSection(Stream* stream);
   void loadLayerInformation(Stream* stream);

   Header _header;
   int32_t _layer_count = 0;
   std::unique_ptr<Layer[]> _layers;
};
