#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

class Stream
{
public:
   Stream() = default;
   virtual ~Stream() = default;

   virtual const std::string& getPath() const;

   int32_t getEndian() const;           // get current endian mode
   void setEndian(int32_t big_endian);  // set endian mode

   virtual void getData(std::span<std::byte> destination) = 0;  // fill "destination"
   virtual void writeData(std::span<const std::byte> source) = 0;

   virtual char getChar();           // get single character
   virtual uint8_t getByte();        // get single byte
   virtual int32_t getWord();        // get unsigned word (2 byte)
   virtual int16_t getShort();       // get short (2 byte)
   virtual int32_t getInt();         // get integer (4 byte)
   virtual float getFloat();         // get float (4 byte)
   virtual std::string getString();  // get string (0 terminated)

   // length byte followed by the characters, trailing \0 characters stripped
   std::string getPrefixedString();

   virtual void writeChar(char);                     // write single character
   virtual void writeByte(uint8_t);                  // write single byte
   virtual void writeWord(uint16_t);                 // write word (2 byte)
   virtual void writeShort(int16_t);                 // write short
   virtual void writeInt(int32_t);                   // write integer (4 byte)
   virtual void writeFloat(float);                   // write float (4 byte)
   virtual void writeString(std::string_view text);  // write string (0 terminated)

   // length byte (at most 255) followed by the characters
   void writePrefixedString(std::string_view text);

   virtual int32_t pos() const;
   virtual void skip(int32_t size);

   // raw bytes of a trivially copyable value, no endian conversion
   template <class Value>
   Value getRaw()
   {
      std::array<std::byte, sizeof(Value)> bytes{};
      getData(bytes);
      return std::bit_cast<Value>(bytes);
   }

   template <class Value>
   void writeRaw(const Value& value)
   {
      const auto bytes = std::bit_cast<std::array<std::byte, sizeof(Value)>>(value);
      writeData(bytes);
   }

private:
   static constexpr int32_t kMachineEndian = (std::endian::native == std::endian::big) ? 1 : 0;

   int32_t _endian = 0;

protected:
   int32_t _position = 0;
};

Stream& operator<<(Stream& stream, float&);
void operator<<(float&, Stream& stream);
Stream& operator<<(Stream& stream, int32_t&);

void operator>>(float&, Stream& stream);

// int32 element count followed by the elements, each read with "item << stream"
template <class Item>
void loadList(Stream& stream, std::vector<Item>& list)
{
   const int32_t size = stream.getInt();
   list.clear();
   list.resize(static_cast<size_t>(std::max(size, 0)));
   for (auto& item : list)
   {
      item << stream;
   }
}

template <class Item>
void writeList(Stream& stream, std::vector<Item>& list)
{
   stream.writeInt(static_cast<int32_t>(list.size()));
   for (auto& item : list)
   {
      item >> stream;
   }
}
