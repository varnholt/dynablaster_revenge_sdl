// base class for post production filters

#pragma once

#include <cstdint>
#include <string>

class Filter
{
public:
   Filter(const std::string& name);
   virtual ~Filter() = default;

   virtual bool init() = 0;
   virtual void process(uint32_t texture, float u, float v) = 0;

   const std::string& getName() const;

private:
   std::string _name;
};
