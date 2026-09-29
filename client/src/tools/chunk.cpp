#include "chunk.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

Chunk::Chunk(Stream* stream) : _stream(stream)
{
   _id = _stream->getInt();
   if (_id != 0xffff)
   {
      ObjectName::load(_stream);
      _size = _stream->getInt();
      if (_size < 0)
      {
         std::printf("break!\n");
      }
   }
   else
   {
      _size = 0;
   }

   _chunk_position = _stream->pos();
}

Chunk::Chunk(Stream* stream, int32_t id, const String& name) : ObjectName(name), _stream(stream), _mode(AccessMode::Write), _id(id)
{
}

Chunk::~Chunk()
{
   if (_mode != AccessMode::Write)
   {
      return;
   }

   _stream->writeInt(_id);
   ObjectName::write(_stream);

   // every buffer but the last is full; the last one is only partially filled unless it filled up exactly
   const auto buffer_length = [this](size_t index)
   {
      const bool last = (index == _buffers.size() - 1);
      return (last && _buffer) ? _buffer_position : kBufferSize;
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
   _stream->writeInt(size);

   for (size_t i = 0; i < _buffers.size(); i++)
   {
      _stream->writeData(_buffers[i]->data(), buffer_length(i));
   }
}

int32_t Chunk::id() const
{
   return _id;
}

int32_t Chunk::dataLeft() const
{
   return _size - _stream->pos() + _chunk_position;
}

void Chunk::skip()
{
   _stream->skip(_size - _stream->pos() + _chunk_position);
}

void Chunk::getData(void* destination, int32_t size)
{
   _stream->getData(destination, size);
   _position += size;
}

void Chunk::writeData(void* data, int32_t size)
{
   const auto* source = static_cast<const char*>(data);
   while (size > 0)
   {
      if (!_buffer)
      {
         _buffers.push_back(std::make_unique<Buffer>());
         _buffer = _buffers.back().get();
         _buffer_position = 0;
      }

      const int32_t length = std::min(size, kBufferSize - _buffer_position);
      std::memcpy(_buffer->data() + _buffer_position, source, length);
      source += length;
      _buffer_position += length;

      if (_buffer_position >= kBufferSize)
      {
         _buffer = nullptr;
         _buffer_position = 0;
      }

      size -= length;
   }
}
