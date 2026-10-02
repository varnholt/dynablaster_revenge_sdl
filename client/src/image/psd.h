#pragma once

#include "image/image.h"

#include <array>
#include <cstdint>
#include <istream>
#include <string>
#include <vector>

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

      void load(std::istream& stream);
      int32_t getWidth() const;
      int32_t getHeight() const;

   private:
      std::array<uint8_t, 4> _signature{};  // file id "8BPS"
      uint16_t _version = 0;                // always 1
      std::array<uint8_t, 6> _reserved{};   // must be zeroed
      uint16_t _channels = 0;               // color channels (1-24) including alpha channels
      int32_t _height = 0;                  // 1-30000
      int32_t _width = 0;                   // 1-30000
      uint16_t _depth = 0;                  // bits per channel (1, 8 and 16)
      uint16_t _mode = 0;                   // ColorMode
   };

   class Layer
   {
   public:
      //! photoshop's layer group markers: a group is stored as its bounding divider, its layers
      //! and finally the folder layer carrying the group's name
      enum class SectionDivider
      {
         None = -1,
         Any = 0,
         OpenFolder = 1,
         ClosedFolder = 2,
         BoundingSectionDivider = 3,
      };

      class Channel
      {
      public:
         void load(std::istream& stream);
         void init(int16_t id, int32_t width, int32_t height);
         void loadRLE(int32_t width, int32_t height, std::istream& stream);
         void loadRaw(int32_t width, int32_t height, std::istream& stream);
         const uint8_t* getScanline(int32_t y) const;
         int16_t getID() const;

      private:
         int16_t _id = 0;
         int32_t _size = 0;
         int32_t _width = 0;
         std::vector<uint8_t> _data;
      };

      void loadLayerRecords(std::istream& stream);
      void loadChannelImageData(std::istream& stream);

      int32_t getBottom() const;
      int32_t getTop() const;
      int32_t getLeft() const;
      int32_t getRight() const;
      int32_t getWidth() const;
      int32_t getHeight() const;
      void setOpacity(int32_t opacity);
      int32_t getOpacity() const;
      bool isVisible() const;
      void setVisible(bool visible);
      const std::string& getName() const;
      void setName(const std::string& name);
      void move(int32_t x, int32_t y);
      void setX(int32_t x);
      void setY(int32_t y);
      void setBottom(int32_t value);
      void setTop(int32_t value);
      const Image& getImage() const;
      void setImage(const Image& image);

      SectionDivider getSectionDivider() const;
      bool isSectionDivider() const;

      //! a layer with pixels, not a group marker
      bool isImageLayer() const;

      //! name of the innermost group the layer is in, empty at top level
      const std::string& getGroup() const;

   private:
      const Channel& getChannel(int16_t id) const;

      int32_t _top = 0;
      int32_t _left = 0;
      int32_t _bottom = 0;
      int32_t _right = 0;
      uint16_t _channel_count = 0;
      Image _image;                    // copies share the pixels
      std::vector<Channel> _channels;  // only valid while loading
      std::array<uint8_t, 4> _blend_mode_signature{};
      std::array<uint8_t, 4> _blend_mode_key{};
      uint8_t _opacity = 0;
      uint8_t _clipping = 0;
      uint8_t _flags = 0;
      std::string _name;
      SectionDivider _section_divider = SectionDivider::None;
      std::string _group;

      friend class PSD;
   };

   int32_t getWidth() const;
   int32_t getHeight() const;
   size_t getLayerCount() const;

   const std::vector<Layer>& getLayers() const;
   std::vector<Layer>& getLayers();
   const Layer& getLayer(size_t index) const;
   Layer& getLayer(size_t index);

   //! first layer with the given name, getLayers().end() if there is none
   std::vector<Layer>::const_iterator getLayer(const std::string& name) const;
   std::vector<Layer>::iterator getLayer(const std::string& name);

   //! appends a layer on top of all others; invalidates references to existing layers
   void addLayer(Layer layer);

   //! resolves the filename against the FileStream search paths
   bool load(const std::string& filename);
   void load(std::istream& stream);

   static std::string loadString(std::istream& stream);

private:
   void loadColorModeData(std::istream& stream);
   void loadImageResourceSection(std::istream& stream);
   void loadLayerAndMaskInformation(std::istream& stream);
   void assignGroups();

   Header _header;
   std::vector<Layer> _layers;
};
