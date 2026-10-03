#include "uvchannel.h"
#include "tools/stream.h"

UVChannel::UVChannel(int32_t id, std::span<const UV> uv) : _id(id), _uv(uv.begin(), uv.end())
{
}

int32_t UVChannel::id() const
{
   return _id;
}

UV* UVChannel::data()
{
   return _uv.empty() ? nullptr : _uv.data();
}

const std::vector<UV>& UVChannel::getUV() const
{
   return _uv;
}

std::vector<UV>& UVChannel::getUV()
{
   return _uv;
}

void UVChannel::load(Stream& stream)
{
   _id = stream.getInt();

   loadList(stream, _uv);
}

void UVChannel::write(Stream& stream)
{
   stream.writeInt(_id);

   writeList(stream, _uv);
}
