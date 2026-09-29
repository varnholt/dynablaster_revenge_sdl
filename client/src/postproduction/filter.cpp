#include "filter.h"

Filter::Filter(const String& name) : _name(name)
{
}

const String& Filter::getName() const
{
   return _name;
}
