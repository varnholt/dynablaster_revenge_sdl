#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

// cache simulation
class VCache
{
public:
   VCache(int32_t size) : _data(static_cast<size_t>(size), 0xffff)
   {
   }

   bool add(uint16_t index)
   {
      if (exist(index))
      {
         return true;
      }

      // kick oldest element
      std::shift_left(_data.begin(), _data.end(), 1);

      // add new index
      _data.back() = index;

      return false;
   }

   bool exist(uint16_t index) const
   {
      return std::ranges::find(_data, index) != _data.end();
   }

   uint16_t get(int32_t index) const
   {
      return _data[index];
   }

   int32_t size() const
   {
      return static_cast<int32_t>(_data.size());
   }

   uint16_t* data()
   {
      return _data.data();
   }

private:
   std::vector<uint16_t> _data;
};
