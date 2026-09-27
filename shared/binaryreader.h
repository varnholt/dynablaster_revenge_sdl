#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "point.h"

// reads fixed-width, little-endian values back out of a byte buffer, mirroring
// QDataStream's operator>> shape; a read past the end leaves the value untouched
// and latches 'ok' to false instead of asserting, since packet bytes come off the
// network and a truncated/malformed packet must fail gracefully, not crash.
class BinaryReader
{
public:
   BinaryReader(const uint8_t* data, size_t size);
   explicit BinaryReader(std::span<const uint8_t> buffer);
   explicit BinaryReader(const std::vector<uint8_t>& buffer);

   BinaryReader& operator>>(int8_t& value);
   BinaryReader& operator>>(uint8_t& value);
   BinaryReader& operator>>(int16_t& value);
   BinaryReader& operator>>(uint16_t& value);
   BinaryReader& operator>>(int32_t& value);
   BinaryReader& operator>>(uint32_t& value);
   BinaryReader& operator>>(int64_t& value);
   BinaryReader& operator>>(uint64_t& value);
   BinaryReader& operator>>(float& value);
   BinaryReader& operator>>(double& value);
   BinaryReader& operator>>(bool& value);
   BinaryReader& operator>>(std::string& value);
   BinaryReader& operator>>(Point& value);

   template <typename T>
   BinaryReader& operator>>(std::vector<T>& list)
   {
      uint32_t count = 0;
      *this >> count;

      list.clear();

      for (uint32_t i = 0; i < count && _ok; ++i)
      {
         T item{};
         *this >> item;
         list.push_back(item);
      }

      return *this;
   }

   [[nodiscard]] bool ok() const;
   [[nodiscard]] size_t pos() const;
   [[nodiscard]] size_t bytesAvailable() const;

private:
   template <typename T>
   T read();

   const uint8_t* _data;
   size_t _size;
   size_t _pos = 0;
   bool _ok = true;
};
