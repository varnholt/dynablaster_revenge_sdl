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
      return _data[index];
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
   // kept as _data/_size/_count: derived classes outside tools/ access them directly
   Item* _data = nullptr;  // array of items
   int32_t _size = 0;      // capacity of array
   int32_t _count = 0;     // number of items in array
};

template <class Item>
Array<Item>::Array(int32_t size) : _size(size)
{
   if (_size > 0)
   {
      _data = new Item[_size];
   }
}

template <class Item>
Array<Item>::Array(const Array& other) : Referenced(other), _data(other.data()), _size(other.capacity()), _count(other.size())
{
}

template <class Item>
Array<Item>::Array(Item* items, int32_t count) : _size(count), _count(count)
{
   if (count > 0)
   {
      _data = new Item[count];
      std::copy_n(items, count, _data);
   }
}

// delete array if no more references
template <class Item>
Array<Item>::~Array()
{
   if (getRefCount() == 1)
   {
      delete[] _data;
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
         delete[] _data;
         delete _references;
      }

      _references = other.getRef();
      addRef();
      _size = other.capacity();
      _count = other.size();
      _data = other.data();
   }
   return *this;
}

template <class Item>
void Array<Item>::init(int32_t size)
{
   // data is not referenced by another object? delete it.
   if (!copyRef())
   {
      delete[] _data;
   }

   _data = (size > 0) ? new Item[size] : nullptr;
   _size = size;
   _count = 0;
}

template <class Item>
void Array<Item>::clear()
{
   if (copyRef())
   {
      _data = new Item[_size];
   }
   _count = 0;
}

template <class Item>
const Item& Array<Item>::get(int32_t index) const
{
   return _data[index];
}

template <class Item>
const Item& Array<Item>::getLast() const
{
   return _data[_count - 1];
}

template <class Item>
const Item& Array<Item>::takeLast()
{
   _count--;
   return _data[_count];
}

template <class Item>
int32_t Array<Item>::add(const Item& item)
{
   if (copyRef())
   {
      deepCopy(*this);
   }
   else if (_count >= _size)
   {
      // grow array when more elements are needed
      Item* previous = _data;
      _size += kGrowArray;
      _data = new Item[_size];
      if (previous)
      {
         std::copy_n(previous, _count, _data);
         delete[] previous;
      }
   }

   const int32_t index = _count;
   _data[_count] = item;
   _count++;
   return index;
}

template <class Item>
int32_t Array<Item>::add(const Array<Item>& other)
{
   if (copyRef())
   {
      deepCopy(*this);
   }
   else if (_count + other.size() >= _size)
   {
      // grow array when more elements are needed
      Item* previous = _data;
      _size = _count + other.size();
      _data = new Item[_size];
      if (previous)
      {
         std::copy_n(previous, _count, _data);
         delete[] previous;
      }
   }

   for (int32_t i = 0; i < other.size(); i++)
   {
      _data[_count + i] = other[i];
   }

   _count += other.size();

   return -1;
}

template <class Item>
int32_t Array<Item>::indexOf(const Item& item) const
{
   for (int32_t index = 0; index < _count; index++)
   {
      if (_data[index] == item)
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
      const Item* shared = _data;
      _data = new Item[_size];
      std::copy_n(shared, index, _data);
      std::copy(shared + index + 1, shared + _count, _data + index);
   }
   else
   {
      std::copy(_data + index + 1, _data + _count, _data + index);
   }

   _count--;
}

template <class Item>
void Array<Item>::copy(const Array<Item>& other)
{
   if (!copyRef())
   {
      delete[] _data;
   }

   _count = _size = other.size();
   if (_count > 0)
   {
      _data = new Item[_count];
      for (int32_t i = 0; i < _count; i++)
      {
         _data[i] = other[i];
      }
   }
   else
   {
      _data = nullptr;
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
   for (int32_t i = 0; i < _count; i++)
   {
      if (_data[i] != item)
      {
         if (position != i)
         {
            _data[position] = _data[i];
         }
         position++;
      }
   }
   const bool result = (_count != position);
   _count = position;
   return result;
}

template <class Item>
Item* Array<Item>::data() const
{
   return _data;
}

template <class Item>
int32_t Array<Item>::size() const
{
   return _count;
}

template <class Item>
int32_t Array<Item>::capacity() const
{
   return _size;
}

template <class Item>
void Array<Item>::deepCopy(const Array<Item>& other)
{
   const Item* shared = other.data();
   _count = other.size();
   _size = _count + kGrowArray;
   _data = new Item[_size];
   std::copy_n(shared, _count, _data);
}
