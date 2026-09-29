// stream class
// streams from memory resource

#pragma once

#include <cstdint>

#include "stream.h"

class MemoryStream : public Stream
{
public:
   MemoryStream() = default;

   int32_t open(void* buffer, int32_t size);
   void close();

   void getData(void* destination, int32_t size) override;  // get "size" bytes and store in "destination"
   void writeData(void* source, int32_t size) override;     // write "size" bytes

   void skip(int32_t size) override;  // skip "size" bytes
   int32_t size() const;              // total size of the input data
   int32_t pos() const override;      // current position in the input data

private:
   uint8_t* _buffer = nullptr;  // input data (not owned)
   int32_t _size = 0;           // size of the data
};
