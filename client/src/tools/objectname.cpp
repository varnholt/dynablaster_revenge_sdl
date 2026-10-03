#include "objectname.h"
#include "stream.h"

ObjectName::ObjectName(const std::string& name) : _name(name)
{
}

const std::string& ObjectName::name() const
{
   return _name;
}

void ObjectName::setName(const std::string& name)
{
   _name = name;
}

void ObjectName::load(Stream& stream)
{
   _name = stream.getPrefixedString();
}

void ObjectName::write(Stream& stream)
{
   stream.writePrefixedString(_name);
}
