#include "filestream.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
constexpr int32_t kStreamCacheSize = 4096;

std::vector<String> path_list;
}  // namespace

FileStream::FileStream() : _cache_buffer(kStreamCacheSize)
{
   if (path_list.empty())
   {
      addPath(".");
   }
}

FileStream::~FileStream()
{
   close();
}

const String& FileStream::getPath() const
{
   return _path;
}

void FileStream::addPath(const String& path)
{
   path_list.push_back(path);
}

void FileStream::removePath(const String& path)
{
   std::erase(path_list, path);
}

void FileStream::close()
{
   if (_file)
   {
      std::fclose(_file);
      _file = nullptr;
   }

   _cache_position = 0;
   _cache_left = 0;
   _global_position = 0;
   _size = 0;
}

int32_t FileStream::open(const String& name, bool write)
{
   _file = nullptr;

   if (write)
   {
      _file = std::fopen(name, "wb");
      return _file != nullptr;
   }

   // try all paths (last one first)
   for (auto path = path_list.rbegin(); path != path_list.rend() && !_file; ++path)
   {
      _path = *path + "/" + name;
      _file = std::fopen(_path, "rb");
   }

   if (!_file)
   {
      _path = name;
      _file = std::fopen(_path, "rb");
      if (!_file)
      {
         return 0;
      }
   }

   std::fseek(_file, 0, SEEK_END);
   _size = static_cast<int32_t>(std::ftell(_file));
   std::fseek(_file, 0, SEEK_SET);

   _cache_position = 0;
   _cache_left = 0;
   _global_position = 0;
   refill();

   return 1;
}

int32_t FileStream::size() const
{
   return _size;
}

int32_t FileStream::pos() const
{
   return _global_position + _cache_position;
}

void FileStream::getData(void* buffer, int32_t size)
{
   auto* destination = static_cast<uint8_t*>(buffer);
   while (size > 0)
   {
      const int32_t length = std::min(size, _cache_left);
      std::memcpy(destination, _cache_buffer.data() + _cache_position, length);
      destination += length;
      _cache_position += length;
      _cache_left -= length;
      if (_cache_left <= 0)
      {
         refill();
      }
      size -= length;
   }
}

void FileStream::writeData(void* buffer, int32_t size)
{
   std::fwrite(buffer, 1, size, _file);
}

void FileStream::skip(int32_t size)
{
   while (size > 0)
   {
      const int32_t length = std::min(_cache_left, size);
      _cache_position += length;
      _cache_left -= length;
      size -= length;
      if (_cache_left <= 0)
      {
         refill();
      }
   }
}

int32_t FileStream::refill()
{
   _global_position += _cache_position;

   if (_cache_left)
   {
      // copy remaining data to front, then fill up the remaining space
      std::memmove(_cache_buffer.data(), _cache_buffer.data() + _cache_position, _cache_left);
      _cache_left += static_cast<int32_t>(std::fread(_cache_buffer.data() + _cache_left, 1, kStreamCacheSize - _cache_left, _file));
   }
   else
   {
      _cache_left = static_cast<int32_t>(std::fread(_cache_buffer.data(), 1, kStreamCacheSize, _file));
   }

   _cache_position = 0;

   return _cache_left;
}
