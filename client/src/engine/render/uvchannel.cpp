#include "uvchannel.h"
#include "tools/stream.h"

UVChannel::UVChannel(int32_t id, UV* uv, int32_t size) : _id(id)
{
   _uv.init(size);
   for (int32_t i = 0; i < size; i++)
   {
      _uv.add(uv[i]);
   }
}

int32_t UVChannel::id() const
{
   return _id;
}

UV* UVChannel::data() const
{
   return _uv.data();
}

const List<UV>& UVChannel::getUV() const
{
   return _uv;
}

void UVChannel::load(Stream* stream)
{
   _id = stream->getInt();

   _uv << *stream;
}

void UVChannel::write(Stream* stream)
{
   stream->writeInt(_id);

   _uv >> *stream;
}

void UVChannel::copy(const UVChannel& other)
{
   _id = other.id();
   _uv.copy(other.getUV());
}
