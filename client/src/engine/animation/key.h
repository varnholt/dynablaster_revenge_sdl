// template class to represent a single animation key of given template type

#pragma once

#include "keybase.h"
#include "tools/stream.h"

template <class Item>
class Key : public KeyBase
{
public:
   Key() = default;
   Key(int32_t time, const Item& value);

   const Item& value() const;
   void setValue(const Item& value);

   void load(Stream* stream) override;
   void write(Stream* stream) override;

protected:
   Item _value{};
};

template <class Item>
Key<Item>::Key(int32_t time, const Item& value) : KeyBase(time), _value(value)
{
}

template <class Item>
const Item& Key<Item>::value() const
{
   return _value;
}

template <class Item>
void Key<Item>::setValue(const Item& value)
{
   _value = value;
}

template <class Item>
void Key<Item>::load(Stream* stream)
{
   KeyBase::load(stream);
   _value << *stream;
}

template <class Item>
void Key<Item>::write(Stream* stream)
{
   KeyBase::write(stream);
   _value >> *stream;
}
