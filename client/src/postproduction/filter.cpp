#include "filter.h"

Filter::Filter(const std::string& name) : _name(name)
{
}

const std::string& Filter::getName() const
{
   return _name;
}
