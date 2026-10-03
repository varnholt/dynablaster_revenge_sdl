#pragma once

// lazily created global instance.
// it is never destroyed: textures, images and timers held by other statics still call back into
// their pools while those statics are torn down at exit.
template <class Item>
class Singleton
{
protected:
   Singleton() = default;
   virtual ~Singleton() = default;

public:
   static Item& Instance()
   {
      static Storage storage;
      return storage._item;
   }

private:
   union Storage
   {
      Storage() : _item()
      {
      }

      ~Storage()
      {
      }

      Item _item;
   };
};
