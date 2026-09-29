#pragma once

#include <memory>
#include <string>
#include <unordered_map>

// owns every item added to it
template <class Item>
class Pool
{
public:
   virtual ~Pool() = default;

   Item* get(const char* id) const;

   // takes ownership of "item" when added; returns false (ownership stays with the caller) if "id" exists
   bool add(const char* id, Item* item);

private:
   std::unordered_map<std::string, std::unique_ptr<Item>> _data;
};

template <class Item>
Item* Pool<Item>::get(const char* id) const
{
   const auto iterator = _data.find(id);
   if (iterator != _data.end())
   {
      return iterator->second.get();
   }
   return nullptr;
}

template <class Item>
bool Pool<Item>::add(const char* id, Item* item)
{
   if (_data.contains(id))
   {
      return false;
   }
   _data.emplace(std::string(id), std::unique_ptr<Item>(item));
   return true;
}
