#include "filestream.h"

#include <algorithm>
#include <vector>

namespace
{
constexpr int32_t kStreamCacheSize = 4096;

std::vector<std::string> path_list;
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

const std::string& FileStream::getPath() const
{
   return _path;
}

void FileStream::addPath(const std::string& path)
{
   path_list.push_back(path);
}

void FileStream::removePath(const std::string& path)
{
   std::erase(path_list, path);
}

void FileStream::close()
{
   _file.reset();

   _cache_position = 0;
   _cache_left = 0;
   _global_position = 0;
   _size = 0;
}

int32_t FileStream::open(const std::string& name, bool write)
{
   _file.reset();

   if (write)
   {
      _file.reset(std::fopen(name.c_str(), "wb"));
      return _file != nullptr;
   }

   // try all paths (last one first)
   for (auto path = path_list.rbegin(); path != path_list.rend() && !_file; ++path)
   {
      _path = *path + "/" + name;
      _file.reset(std::fopen(_path.c_str(), "rb"));
   }

   if (!_file)
   {
      _path = name;
      _file.reset(std::fopen(_path.c_str(), "rb"));
      if (!_file)
      {
         return 0;
      }
   }

   std::fseek(_file.get(), 0, SEEK_END);
   _size = static_cast<int32_t>(std::ftell(_file.get()));
   std::fseek(_file.get(), 0, SEEK_SET);

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

void FileStream::getData(std::span<std::byte> destination)
{
   while (!destination.empty())
   {
      const auto length = static_cast<size_t>(std::min(static_cast<int32_t>(destination.size()), _cache_left));
      std::ranges::copy(std::span(_cache_buffer).subspan(_cache_position, length), destination.begin());
      destination = destination.subspan(length);
      _cache_position += static_cast<int32_t>(length);
      _cache_left -= static_cast<int32_t>(length);
      if (_cache_left <= 0)
      {
         refill();
      }
   }
}

void FileStream::writeData(std::span<const std::byte> source)
{
   std::fwrite(source.data(), 1, source.size(), _file.get());
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
      const auto remaining = _cache_buffer.begin() + _cache_position;
      std::copy(remaining, remaining + _cache_left, _cache_buffer.begin());
      const auto free_space = std::span(_cache_buffer).subspan(_cache_left);
      _cache_left += static_cast<int32_t>(std::fread(free_space.data(), 1, free_space.size(), _file.get()));
   }
   else
   {
      _cache_left = static_cast<int32_t>(std::fread(_cache_buffer.data(), 1, kStreamCacheSize, _file.get()));
   }

   _cache_position = 0;

   return _cache_left;
}
