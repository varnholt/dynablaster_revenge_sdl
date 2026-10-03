#include "stream.h"

#include <algorithm>
#include <array>

const std::string& Stream::getPath() const
{
   static const std::string dummy;
   return dummy;
}

Stream& operator<<(Stream& stream, float& value)
{
   value = stream.getFloat();
   return stream;
}

void operator<<(float& value, Stream& stream)
{
   value = stream.getFloat();
}

Stream& operator<<(Stream& stream, int32_t& value)
{
   value = stream.getInt();
   return stream;
}

void operator>>(float& value, Stream& stream)
{
   stream.writeFloat(value);
}

int32_t Stream::getEndian() const
{
   return _endian;
}

void Stream::setEndian(int32_t big_endian)
{
   _endian = big_endian;
}

char Stream::getChar()
{
   return getRaw<char>();
}

uint8_t Stream::getByte()
{
   return getRaw<uint8_t>();
}

int32_t Stream::getInt()
{
   const auto value = getRaw<int32_t>();
   return (_endian != kMachineEndian) ? std::byteswap(value) : value;
}

int32_t Stream::getWord()
{
   const auto word = getRaw<uint16_t>();
   return (_endian != kMachineEndian) ? std::byteswap(word) : word;
}

int16_t Stream::getShort()
{
   const auto word = getRaw<int16_t>();
   return (_endian != kMachineEndian) ? std::byteswap(word) : word;
}

float Stream::getFloat()
{
   const auto bits = getRaw<uint32_t>();
   return std::bit_cast<float>((_endian != kMachineEndian) ? std::byteswap(bits) : bits);
}

std::string Stream::getString()
{
   // read chars until (exclusive) 0-terminator
   std::string result;
   for (char c = getChar(); c != 0; c = getChar())
   {
      result.push_back(c);
   }
   return result;
}

std::string Stream::getPrefixedString()
{
   std::string result(getByte(), '\0');
   getData(std::as_writable_bytes(std::span(result)));

   while (!result.empty() && result.back() == 0)
   {
      result.pop_back();
   }
   return result;
}

void Stream::skip(int32_t size)
{
   std::array<std::byte, 256> dummy{};
   while (size > 0)
   {
      const int32_t length = std::min<int32_t>(size, static_cast<int32_t>(dummy.size()));
      getData(std::span(dummy).first(length));
      size -= length;
   }
}

int32_t Stream::pos() const
{
   return _position;
}

void Stream::writeChar(char c)
{
   writeRaw(c);
}

void Stream::writeByte(uint8_t value)
{
   writeRaw(value);
}

void Stream::writeWord(uint16_t word)
{
   writeRaw((_endian != kMachineEndian) ? std::byteswap(word) : word);
}

void Stream::writeShort(int16_t word)
{
   writeRaw((_endian != kMachineEndian) ? std::byteswap(word) : word);
}

void Stream::writeInt(int32_t value)
{
   writeRaw((_endian != kMachineEndian) ? std::byteswap(value) : value);
}

void Stream::writeFloat(float value)
{
   const auto bits = std::bit_cast<uint32_t>(value);
   writeRaw((_endian != kMachineEndian) ? std::byteswap(bits) : bits);
}

void Stream::writeString(std::string_view text)
{
   for (const char c : text)
   {
      writeChar(c);
   }
   writeChar(0);
}

void Stream::writePrefixedString(std::string_view text)
{
   const auto length = std::min<size_t>(text.size(), 255);
   writeByte(static_cast<uint8_t>(length));
   writeData(std::as_bytes(std::span(text.substr(0, length))));
}
