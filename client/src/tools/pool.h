#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

// owns every item added to it
template <class Item>
class Pool
{
public:
   virtual ~Pool() = default;

   std::optional<std::reference_wrapper<Item>> get(const std::string& id) const;

   // keeps the existing item if "id" is already taken
   Item& add(const std::string& id, std::unique_ptr<Item> item);

private:
   std::unordered_map<std::string, std::unique_ptr<Item>> _data;
};

template <class Item>
std::optional<std::reference_wrapper<Item>> Pool<Item>::get(const std::string& id) const
{
   const auto iterator = _data.find(id);
   if (iterator != _data.end())
   {
      return *iterator->second;
   }
   return std::nullopt;
}

template <class Item>
Item& Pool<Item>::add(const std::string& id, std::unique_ptr<Item> item)
{
   return *_data.try_emplace(id, std::move(item)).first->second;
}
