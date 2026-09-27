#include "binarywriter.h"

#include <cstring>

BinaryWriter::BinaryWriter(std::vector<uint8_t>& buffer) : _buffer(buffer)
{
}

template <typename T>
void BinaryWriter::write(T value)
{
   const auto offset = _buffer.size();
   _buffer.resize(offset + sizeof(T));
   std::memcpy(_buffer.data() + offset, &value, sizeof(T));
}

BinaryWriter& BinaryWriter::operator<<(int8_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(uint8_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(int16_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(uint16_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(int32_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(uint32_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(int64_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(uint64_t value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(float value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(double value)
{
   write(value);
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(bool value)
{
   write(static_cast<uint8_t>(value ? 1 : 0));
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(const std::string& value)
{
   write(static_cast<uint32_t>(value.size()));
   const auto offset = _buffer.size();
   _buffer.resize(offset + value.size());
   std::memcpy(_buffer.data() + offset, value.data(), value.size());
   return *this;
}

BinaryWriter& BinaryWriter::operator<<(const Point& value)
{
   *this << static_cast<int32_t>(value.x());
   *this << static_cast<int32_t>(value.y());
   return *this;
}

size_t BinaryWriter::pos() const
{
   return _buffer.size();
}

void BinaryWriter::patchUint16(size_t offset, uint16_t value)
{
   std::memcpy(_buffer.data() + offset, &value, sizeof(value));
}
