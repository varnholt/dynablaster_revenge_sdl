#include "stream.h"

#include <algorithm>
#include <array>
#include <utility>

#include "string.h"

const String& Stream::getPath() const
{
   static String dummy;
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

void Stream::swap16(void* data)
{
   auto* bytes = static_cast<uint8_t*>(data);
   std::swap(bytes[0], bytes[1]);
}

void Stream::swap32(void* data)
{
   auto* bytes = static_cast<uint8_t*>(data);
   std::swap(bytes[0], bytes[3]);
   std::swap(bytes[1], bytes[2]);
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
   char c = 0;
   getData(&c, sizeof(char));
   return c;
}

uint8_t Stream::getByte()
{
   uint8_t c = 0;
   getData(&c, sizeof(uint8_t));
   return c;
}

int32_t Stream::getInt()
{
   int32_t value = 0;
   getData(&value, sizeof(int32_t));
   if (_endian != kMachineEndian)
   {
      swap32(&value);
   }
   return value;
}

int32_t Stream::getWord()
{
   uint16_t word = 0;
   getData(&word, sizeof(uint16_t));
   if (_endian != kMachineEndian)
   {
      swap16(&word);
   }
   return word;
}

int16_t Stream::getShort()
{
   int16_t word = 0;
   getData(&word, sizeof(int16_t));
   if (_endian != kMachineEndian)
   {
      swap16(&word);
   }
   return word;
}

float Stream::getFloat()
{
   float value = 0.0f;
   getData(&value, sizeof(float));
   if (_endian != kMachineEndian)
   {
      swap32(&value);
   }
   return value;
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

void Stream::skip(int32_t size)
{
   std::array<char, 256> dummy{};
   while (size > 0)
   {
      const int32_t length = std::min<int32_t>(size, static_cast<int32_t>(dummy.size()));
      getData(dummy.data(), length);
      size -= length;
   }
}

int32_t Stream::pos() const
{
   return _position;
}

void Stream::writeChar(char c)
{
   writeData(&c, sizeof(char));
}

void Stream::writeByte(uint8_t value)
{
   writeData(&value, sizeof(uint8_t));
}

void Stream::writeWord(uint16_t word)
{
   if (_endian != kMachineEndian)
   {
      swap16(&word);
   }
   writeData(&word, sizeof(uint16_t));
}

void Stream::writeShort(int16_t word)
{
   if (_endian != kMachineEndian)
   {
      swap16(&word);
   }
   writeData(&word, sizeof(int16_t));
}

void Stream::writeInt(int32_t value)
{
   if (_endian != kMachineEndian)
   {
      swap32(&value);
   }
   writeData(&value, sizeof(int32_t));
}

void Stream::writeFloat(float value)
{
   if (_endian != kMachineEndian)
   {
      swap32(&value);
   }
   writeData(&value, sizeof(float));
}

void Stream::writeString(const char* data)
{
   if (data)
   {
      for (const char* c = data; *c; ++c)
      {
         writeChar(*c);
      }
   }
   writeChar(0);
}
