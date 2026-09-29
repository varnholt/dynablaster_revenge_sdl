#include "objectname.h"

ObjectName::ObjectName(const String& name) : _name(name)
{
}

const String& ObjectName::name() const
{
   return _name;
}

void ObjectName::setName(const String& name)
{
   _name = name;
}

void ObjectName::load(Stream* stream)
{
   _name.load(stream);
}

void ObjectName::write(Stream* stream)
{
   _name.write(stream);
}
