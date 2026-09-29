#pragma once

// lazily created global instance; a derived object constructed explicitly registers itself instead.
// non-owning: whoever creates the instance (or nobody, for lazily created ones) keeps it alive.
template <class Item>
class Singleton
{
protected:
   Singleton()
   {
      if (!_instance)
      {
         _instance = static_cast<Item*>(this);
      }
   }

   virtual ~Singleton()
   {
      _instance = nullptr;
   }

public:
   static Item* Instance()
   {
      if (!_instance)
      {
         _instance = new Item();
      }
      return _instance;
   }

   static void cleanUp()
   {
   }

private:
   static inline Item* _instance = nullptr;
};
