// stream class
// performs cached file access
// "stream" is the central management object for getting data from the disk

// todo:
// - handle eof
// - get data from compressed archive

#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "stream.h"
#include "string.h"

class FileStream : public Stream
{
public:
   FileStream();
   ~FileStream() override;

   FileStream(const FileStream&) = delete;
   FileStream& operator=(const FileStream&) = delete;

   static void addPath(const String& path);
   static void removePath(const String& path);

   int32_t open(const String& name, bool write = false);  // open file "name"
   void close();                                          // close stream

   const String& getPath() const override;

   void getData(void* destination, int32_t size) override;  // get "size" bytes and store in "destination"
   void writeData(void* source, int32_t size) override;     // write "size" bytes

   void skip(int32_t size) override;  // skip "size" bytes
   int32_t size() const;              // total size of the input file
   int32_t pos() const override;      // current position in the input file

private:
   int32_t refill();  // refill cache

   FILE* _file = nullptr;               // source file
   std::vector<uint8_t> _cache_buffer;  // input cache
   int32_t _cache_position = 0;         // current position in cache
   int32_t _cache_left = 0;             // data left in cache
   int32_t _global_position = 0;        // absolute position in file (including cache position)
   int32_t _size = 0;                   // size of the file
   String _path;
};
