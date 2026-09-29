// base class for post production filters

#pragma once

#include "tools/string.h"

#include <cstdint>

class Filter
{
public:
   Filter(const String& name);
   virtual ~Filter() = default;

   virtual bool init() = 0;
   virtual void process(uint32_t texture, float u, float v) = 0;

   const String& getName() const;

private:
   String _name;
};
