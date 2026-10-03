#include "stream.h"

#include <algorithm>
#include <ios>

Stream::Stream(std::istream& input) : _stream(std::ref(input))
{
}

Stream::Stream(std::ostream& output) : _stream(std::ref(output))
{
}

std::istream& Stream::input()
{
   return std::get<std::reference_wrapper<std::istream>>(_stream);
}

std::ostream& Stream::output()
{
   return std::get<std::reference_wrapper<std::ostream>>(_stream);
}

void Stream::getData(std::span<std::byte> destination)
{
   input().read(reinterpret_cast<char*>(destination.data()), static_cast<std::streamsize>(destination.size()));
}

void Stream::writeData(std::span<const std::byte> source)
{
   output().write(reinterpret_cast<const char*>(source.data()), static_cast<std::streamsize>(source.size()));
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
   if (size > 0)
   {
      input().seekg(size, std::ios::cur);
   }
}

int32_t Stream::pos() const
{
   using Input = std::reference_wrapper<std::istream>;
   using Output = std::reference_wrapper<std::ostream>;
   if (std::holds_alternative<Input>(_stream))
   {
      return static_cast<int32_t>(std::get<Input>(_stream).get().tellg());
   }
   return static_cast<int32_t>(std::get<Output>(_stream).get().tellp());
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
