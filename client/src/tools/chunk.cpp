#include "chunk.h"

#include <cstdio>
#include <span>

Chunk::Chunk(Stream& stream) : Stream(stream.input()), _stream(stream)
{
   _id = stream.getInt();
   if (_id != 0xffff)
   {
      ObjectName::load(stream);
      _size = stream.getInt();
      if (_size < 0)
      {
         std::printf("break!\n");
      }
   }
   else
   {
      _size = 0;
   }

   _chunk_position = pos();
}

Chunk::Chunk(Stream& stream, int32_t id, const std::string& name)
    : Stream(_buffer), ObjectName(name), _stream(stream), _mode(AccessMode::Write), _id(id)
{
}

Chunk::~Chunk()
{
   if (_mode != AccessMode::Write)
   {
      return;
   }

   Stream& stream = _stream;
   stream.writeInt(_id);
   ObjectName::write(stream);

   const auto data = _buffer.view();
   const auto size = static_cast<int32_t>(data.size());
   if (size < 0)
   {
      std::printf("break!\n");
   }
   stream.writeInt(size);
   stream.writeData(std::as_bytes(std::span(data)));
}

int32_t Chunk::id() const
{
   return _id;
}

int32_t Chunk::dataLeft() const
{
   return _size - pos() + _chunk_position;
}

void Chunk::skip()
{
   Stream::skip(dataLeft());
}
