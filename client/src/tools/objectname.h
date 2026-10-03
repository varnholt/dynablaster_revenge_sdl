#pragma once

#include <string>

class Stream;

class ObjectName
{
public:
   ObjectName() = default;
   ObjectName(const ObjectName& other) = default;
   ObjectName(const std::string& name);
   virtual ~ObjectName() = default;

   const std::string& name() const;
   void setName(const std::string& name);

   void load(Stream& stream);
   void write(Stream& stream);

protected:
   std::string _name;
};
