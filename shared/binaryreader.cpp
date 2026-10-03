#include "binaryreader.h"

#include <algorithm>
#include <array>
#include <bit>

BinaryReader::BinaryReader(std::span<const uint8_t> buffer) : _data(buffer)
{
}

BinaryReader::BinaryReader(const std::vector<uint8_t>& buffer) : _data(buffer)
{
}

template <typename T>
T BinaryReader::read()
{
   T value{};

   if (!_ok)
   {
      return value;
   }

   if (_pos + sizeof(T) > _data.size())
   {
      _ok = false;
      return value;
   }

   std::array<uint8_t, sizeof(T)> bytes{};
   std::ranges::copy(_data.subspan(_pos, sizeof(T)), bytes.begin());
   value = std::bit_cast<T>(bytes);
   _pos += sizeof(T);
   return value;
}

BinaryReader& BinaryReader::operator>>(int8_t& value)
{
   value = read<int8_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(uint8_t& value)
{
   value = read<uint8_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(int16_t& value)
{
   value = read<int16_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(uint16_t& value)
{
   value = read<uint16_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(int32_t& value)
{
   value = read<int32_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(uint32_t& value)
{
   value = read<uint32_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(int64_t& value)
{
   value = read<int64_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(uint64_t& value)
{
   value = read<uint64_t>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(float& value)
{
   value = read<float>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(double& value)
{
   value = read<double>();
   return *this;
}

BinaryReader& BinaryReader::operator>>(bool& value)
{
   value = read<uint8_t>() != 0;
   return *this;
}

BinaryReader& BinaryReader::operator>>(std::string& value)
{
   const auto length = read<uint32_t>();

   if (!_ok || _pos + length > _data.size())
   {
      _ok = false;
      value.clear();
      return *this;
   }

   const auto bytes = _data.subspan(_pos, length);
   value.assign(bytes.begin(), bytes.end());
   _pos += length;
   return *this;
}

BinaryReader& BinaryReader::operator>>(Point& value)
{
   int32_t x = 0;
   int32_t y = 0;
   *this >> x >> y;
   value.setX(x);
   value.setY(y);
   return *this;
}

bool BinaryReader::ok() const
{
   return _ok;
}

size_t BinaryReader::pos() const
{
   return _pos;
}

size_t BinaryReader::bytesAvailable() const
{
   return _data.size() - _pos;
}
