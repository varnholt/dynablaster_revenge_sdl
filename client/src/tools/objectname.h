#pragma once

#include "string.h"

class ObjectName
{
public:
   ObjectName() = default;
   ObjectName(const ObjectName& other) = default;
   ObjectName(const String& name);
   virtual ~ObjectName() = default;

   const String& name() const;
   void setName(const String& name);

   void load(Stream* stream);
   void write(Stream* stream);

protected:
   String _name;
};
