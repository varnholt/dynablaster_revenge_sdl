// template list class. adds load functionality to array<>
#pragma once

#include "array.h"
#include "stream.h"
#include "streamable.h"

template <class Item>
class List : public Array<Item>, public Streamable
{
public:
   List() = default;

   List(Item* items, int32_t count) : Array<Item>(items, count)
   {
   }

   void load(Stream* stream) override
   {
      // shared with another array: detach instead of overwriting (or freeing) the shared items
      if (this->copyRef())
      {
         this->mData = nullptr;
         this->mSize = 0;
         this->mCount = 0;
      }

      const int32_t size = stream->getInt();

      if (size == 0)
      {
         Array<Item>::init(size);
         return;
      }

      // current array not big enough (or not yet initialized): create new
      if (size > this->mSize)
      {
         delete[] this->mData;
         this->mData = new Item[size];
         this->mSize = size;
      }

      for (int32_t i = 0; i < size; i++)
      {
         Item& item = this->mData[i];
         item << *stream;
      }

      this->mCount = size;
   }

   void write(Stream* stream) override
   {
      stream->writeInt(this->size());

      for (int32_t i = 0; i < this->size(); i++)
      {
         Item& item = this->mData[i];
         item >> *stream;
      }
   }
};
