// stream class
// performs cached file access
// "stream" is the central management object for getting data from the disk

// todo:
// - handle eof
// - get data from compressed archive

#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "stream.h"

class FileStream : public Stream
{
public:
   FileStream();
   ~FileStream() override;

   FileStream(const FileStream&) = delete;
   FileStream& operator=(const FileStream&) = delete;

   static void addPath(const std::string& path);
   static void removePath(const std::string& path);

   int32_t open(const std::string& name, bool write = false);  // open file "name"
   void close();                                               // close stream

   const std::string& getPath() const override;

   void getData(std::span<std::byte> destination) override;
   void writeData(std::span<const std::byte> source) override;

   void skip(int32_t size) override;  // skip "size" bytes
   int32_t size() const;              // total size of the input file
   int32_t pos() const override;      // current position in the input file

private:
   struct FileCloser
   {
      void operator()(std::FILE* file) const
      {
         std::fclose(file);
      }
   };

   int32_t refill();  // refill cache

   std::unique_ptr<std::FILE, FileCloser> _file;  // source file
   std::vector<std::byte> _cache_buffer;          // input cache
   int32_t _cache_position = 0;                   // current position in cache
   int32_t _cache_left = 0;                       // data left in cache
   int32_t _global_position = 0;                  // absolute position in file (including cache position)
   int32_t _size = 0;                             // size of the file
   std::string _path;
};
