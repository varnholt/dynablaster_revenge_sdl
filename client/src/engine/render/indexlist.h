#pragma once

#include "tools/array.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <span>

class IndexList : public Array<int32_t>
{
public:
   // sorts descending
   void sort()
   {
      if (_count > 1)
      {
         std::ranges::sort(std::span(_data, static_cast<size_t>(_count)), std::ranges::greater{});
      }
   }

   bool find(int32_t value)
   {
      int32_t l = 0;
      int32_t r = _count - 1;
      while (l <= r)
      {
         const int32_t m = (l + r) >> 1;
         if (_data[m] > value)
         {
            r = m - 1;
         }
         else if (_data[m] < value)
         {
            l = m + 1;
         }
         else
         {
            return true;
         }
      }
      return false;
   }

   // add indices (which are not yet contained) from "list" to this
   int32_t merge(const IndexList& list)
   {
      const int32_t previous_size = size();
      const int32_t count = list.size();
      for (int32_t i = 0; i < count; i++)
      {
         const int32_t index = list.get(i);
         if (!find(index))
         {
            add(index);
         }
      }
      return size() - previous_size;
   }
};
