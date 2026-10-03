#include "chunk.h"

#include <algorithm>
#include <cstdio>

Chunk::Chunk(Stream& stream) : _stream(stream)
{
   _id = _stream.getInt();
   if (_id != 0xffff)
   {
      ObjectName::load(_stream);
      _size = _stream.getInt();
      if (_size < 0)
      {
         std::printf("break!\n");
      }
   }
   else
   {
      _size = 0;
   }

   _chunk_position = _stream.pos();
}

Chunk::Chunk(Stream& stream, int32_t id, const std::string& name) : ObjectName(name), _stream(stream), _mode(AccessMode::Write), _id(id)
{
}

Chunk::~Chunk()
{
   if (_mode != AccessMode::Write)
   {
      return;
   }

   _stream.writeInt(_id);
   ObjectName::write(_stream);

   // every buffer but the last is full; the last one is only partially filled unless it filled up exactly
   const auto buffer_length = [this](size_t index)
   {
      const bool last = (index == _buffers.size() - 1);
      return (last && _buffer_open) ? _buffer_position : kBufferSize;
   };

   int32_t size = 0;
   for (size_t i = 0; i < _buffers.size(); i++)
   {
      size += buffer_length(i);
   }
   if (size < 0)
   {
      std::printf("break!\n");
   }
   _stream.writeInt(size);

   for (size_t i = 0; i < _buffers.size(); i++)
   {
      _stream.writeData(std::span(*_buffers[i]).first(buffer_length(i)));
   }
}

int32_t Chunk::id() const
{
   return _id;
}

int32_t Chunk::dataLeft() const
{
   return _size - _stream.pos() + _chunk_position;
}

void Chunk::skip()
{
   _stream.skip(_size - _stream.pos() + _chunk_position);
}

void Chunk::getData(std::span<std::byte> destination)
{
   _stream.getData(destination);
   _position += static_cast<int32_t>(destination.size());
}

void Chunk::writeData(std::span<const std::byte> source)
{
   while (!source.empty())
   {
      if (!_buffer_open)
      {
         _buffers.push_back(std::make_unique<Buffer>());
         _buffer_open = true;
         _buffer_position = 0;
      }

      const auto length = std::min(source.size(), static_cast<size_t>(kBufferSize - _buffer_position));
      std::ranges::copy(source.first(length), _buffers.back()->begin() + _buffer_position);
      source = source.subspan(length);
      _buffer_position += static_cast<int32_t>(length);

      if (_buffer_position >= kBufferSize)
      {
         _buffer_open = false;
         _buffer_position = 0;
      }
   }
}
