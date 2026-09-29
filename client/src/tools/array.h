/*
 growable array template with reference counting (copies share the data copy-on-write)
 usually the number of stored elements is known in advance and the array will never grow
 whenever the array needs to resize it allocates a small bonus capacity to avoid
 permanent resizing when adding multiple elements
 initially the array has no space allocated (null pointer)

 usage:
 array.init(...) with the number of items required
 array.add(...) items
 array.get(..) item from the array
*/

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "referenced.h"

template <class Item>
class Array : public Referenced
{
public:
   // construct empty array with given capacity (default: zero size, null pointer)
   Array(int32_t size = 0);

   // construct reference of given array "other"
   Array(const Array& other);

   // construct new array from given items and count
   Array(Item* items, int32_t count);

   ~Array() override;

   // reference given array
   Array<Item>& operator=(const Array<Item>& other);

   // initialize array to given size, existing data will be deallocated
   void init(int32_t size);

   // remove all elements from the array. capacity keeps the same, element count is set to 0
   void clear();

   Item& operator[](int32_t index) const
   {
      return mData[index];
   }

   const Item& get(int32_t index) const;

   int32_t add(const Item& item);
   int32_t add(const Array<Item>& other);

   const Item& getLast() const;

   // get and remove last item
   const Item& takeLast();

   // index of element (-1 if not existing)
   int32_t indexOf(const Item& item) const;

   bool contains(const Item& item) const;

   // erase item at given index. following items will be moved
   void erase(int32_t index);

   // create a copy of given array "other"
   void copy(const Array<Item>& other);

   // remove all occurrences of "item", keeping the order of the remaining items
   // non-pointer types need "==" operator
   bool remove(const Item& item);

   Item* data() const;

   // number of items currently in the array
   int32_t size() const;

   // number of items the array can hold without reallocating
   int32_t capacity() const;

private:
   static constexpr int32_t kGrowArray = 10;

   // create a deep copy of given data
   void deepCopy(const Array<Item>& other);

protected:
   // kept as mData/mSize/mCount: derived classes outside tools/ access them directly
   Item* mData = nullptr;  // array of items
   int32_t mSize = 0;      // capacity of array
   int32_t mCount = 0;     // number of items in array
};

template <class Item>
Array<Item>::Array(int32_t size) : mSize(size)
{
   if (mSize > 0)
   {
      mData = new Item[mSize];
   }
}

template <class Item>
Array<Item>::Array(const Array& other) : Referenced(other), mData(other.data()), mSize(other.capacity()), mCount(other.size())
{
}

template <class Item>
Array<Item>::Array(Item* items, int32_t count) : mSize(count), mCount(count)
{
   if (count > 0)
   {
      mData = new Item[count];
      std::copy_n(items, count, mData);
   }
}

// delete array if no more references
template <class Item>
Array<Item>::~Array()
{
   if (getRefCount() == 1)
   {
      delete[] mData;
   }
}

template <class Item>
Array<Item>& Array<Item>::operator=(const Array<Item>& other)
{
   if (this != &other)
   {
      // array is not referenced: delete data and reference counter
      if (getRefCount() == 1)
      {
         delete[] mData;
         delete mReferences;
      }

      mReferences = other.getRef();
      addRef();
      mSize = other.capacity();
      mCount = other.size();
      mData = other.data();
   }
   return *this;
}

template <class Item>
void Array<Item>::init(int32_t size)
{
   // data is not referenced by another object? delete it.
   if (!copyRef())
   {
      delete[] mData;
   }

   mData = (size > 0) ? new Item[size] : nullptr;
   mSize = size;
   mCount = 0;
}

template <class Item>
void Array<Item>::clear()
{
   if (copyRef())
   {
      mData = new Item[mSize];
   }
   mCount = 0;
}

template <class Item>
const Item& Array<Item>::get(int32_t index) const
{
   return mData[index];
}

template <class Item>
const Item& Array<Item>::getLast() const
{
   return mData[mCount - 1];
}

template <class Item>
const Item& Array<Item>::takeLast()
{
   mCount--;
   return mData[mCount];
}

template <class Item>
int32_t Array<Item>::add(const Item& item)
{
   if (copyRef())
   {
      deepCopy(*this);
   }
   else if (mCount >= mSize)
   {
      // grow array when more elements are needed
      Item* previous = mData;
      mSize += kGrowArray;
      mData = new Item[mSize];
      if (previous)
      {
         std::copy_n(previous, mCount, mData);
         delete[] previous;
      }
   }

   const int32_t index = mCount;
   mData[mCount] = item;
   mCount++;
   return index;
}

template <class Item>
int32_t Array<Item>::add(const Array<Item>& other)
{
   if (copyRef())
   {
      deepCopy(*this);
   }
   else if (mCount + other.size() >= mSize)
   {
      // grow array when more elements are needed
      Item* previous = mData;
      mSize = mCount + other.size();
      mData = new Item[mSize];
      if (previous)
      {
         std::copy_n(previous, mCount, mData);
         delete[] previous;
      }
   }

   for (int32_t i = 0; i < other.size(); i++)
   {
      mData[mCount + i] = other[i];
   }

   mCount += other.size();

   return -1;
}

template <class Item>
int32_t Array<Item>::indexOf(const Item& item) const
{
   for (int32_t index = 0; index < mCount; index++)
   {
      if (mData[index] == item)
      {
         return index;
      }
   }
   return -1;
}

template <class Item>
bool Array<Item>::contains(const Item& item) const
{
   return indexOf(item) != -1;
}

template <class Item>
void Array<Item>::erase(int32_t index)
{
   if (copyRef())
   {
      // create new array without element at index
      const Item* shared = mData;
      mData = new Item[mSize];
      std::copy_n(shared, index, mData);
      std::copy(shared + index + 1, shared + mCount, mData + index);
   }
   else
   {
      std::copy(mData + index + 1, mData + mCount, mData + index);
   }

   mCount--;
}

template <class Item>
void Array<Item>::copy(const Array<Item>& other)
{
   if (!copyRef())
   {
      delete[] mData;
   }

   mCount = mSize = other.size();
   if (mCount > 0)
   {
      mData = new Item[mCount];
      for (int32_t i = 0; i < mCount; i++)
      {
         mData[i] = other[i];
      }
   }
   else
   {
      mData = nullptr;
   }
}

template <class Item>
bool Array<Item>::remove(const Item& item)
{
   if (copyRef())
   {
      deepCopy(*this);
   }

   int32_t position = 0;
   for (int32_t i = 0; i < mCount; i++)
   {
      if (mData[i] != item)
      {
         if (position != i)
         {
            mData[position] = mData[i];
         }
         position++;
      }
   }
   const bool result = (mCount != position);
   mCount = position;
   return result;
}

template <class Item>
Item* Array<Item>::data() const
{
   return mData;
}

template <class Item>
int32_t Array<Item>::size() const
{
   return mCount;
}

template <class Item>
int32_t Array<Item>::capacity() const
{
   return mSize;
}

template <class Item>
void Array<Item>::deepCopy(const Array<Item>& other)
{
   const Item* shared = other.data();
   mCount = other.size();
   mSize = mCount + kGrowArray;
   mData = new Item[mSize];
   std::copy_n(shared, mCount, mData);
}
