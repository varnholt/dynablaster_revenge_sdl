#include "idlist.h"

IDList& IDList::operator=(const IDList& other)
{
   init(other.size());
   for (int32_t i = 0; i < other.size(); i++)
   {
      add(other[i]);
   }
   return *this;
}

int32_t IDList::load(Stream* stream)
{
   const int32_t size = stream->getInt();

   init(size);

   for (int32_t i = 0; i < size; i++)
   {
      add(stream->getInt());
   }

   return size;
}
