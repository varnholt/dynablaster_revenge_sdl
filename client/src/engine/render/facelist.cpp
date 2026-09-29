#include "facelist.h"
#include "tools/stream.h"

void FaceList::load(Stream* stream)
{
   mCount = mSize = stream->getInt();
   mData = new uint16_t[mSize];

   for (int32_t i = 0; i < mSize; i++)
   {
      mData[i] = static_cast<uint16_t>(stream->getWord());
   }
}

void FaceList::write(Stream* stream)
{
   stream->writeInt(mCount);

   for (int32_t i = 0; i < mSize; i++)
   {
      stream->writeWord(mData[i]);
   }
}
