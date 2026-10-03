#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

// 2d grid of items; cells outside the grid read as a default constructed item
template <class Item>
class Map2d
{
public:
   // construct Map2d with given size (default: zero size)
   Map2d(int32_t width = 0, int32_t height = 0);

   virtual ~Map2d() = default;

   // initialize Map2d to given size, existing data is dropped
   void init(int32_t width, int32_t height);

   // reset all cells to a default constructed item
   void clear();

   Item get(int32_t x, int32_t y) const;
   void set(int32_t x, int32_t y, const Item& item);

   int32_t width() const;
   int32_t height() const;

protected:
   std::vector<Item> _data;
   int32_t _width = 0;
   int32_t _height = 0;
};

template <class Item>
Map2d<Item>::Map2d(int32_t width, int32_t height)
{
   init(width, height);
}

template <class Item>
void Map2d<Item>::init(int32_t width, int32_t height)
{
   if (width > 0 && height > 0)
   {
      _width = width;
      _height = height;
      _data.assign(static_cast<size_t>(width) * height, Item{});
   }
   else
   {
      _width = 0;
      _height = 0;
      _data.clear();
   }
}

template <class Item>
void Map2d<Item>::clear()
{
   std::ranges::fill(_data, Item{});
}

template <class Item>
Item Map2d<Item>::get(int32_t x, int32_t y) const
{
   if (x >= 0 && x < _width && y >= 0 && y < _height)
   {
      return _data[y * _width + x];
   }
   return Item{};
}

template <class Item>
void Map2d<Item>::set(int32_t x, int32_t y, const Item& item)
{
   if (x >= 0 && x < _width && y >= 0 && y < _height)
   {
      _data[y * _width + x] = item;
   }
}

template <class Item>
int32_t Map2d<Item>::width() const
{
   return _width;
}

template <class Item>
int32_t Map2d<Item>::height() const
{
   return _height;
}
