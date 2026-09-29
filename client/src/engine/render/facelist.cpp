#include "facelist.h"
#include "tools/stream.h"

void FaceList::load(Stream* stream)
{
   _count = _size = stream->getInt();
   _data = new uint16_t[_size];

   for (int32_t i = 0; i < _size; i++)
   {
      _data[i] = static_cast<uint16_t>(stream->getWord());
   }
}

void FaceList::write(Stream* stream)
{
   stream->writeInt(_count);

   for (int32_t i = 0; i < _size; i++)
   {
      stream->writeWord(_data[i]);
   }
}
