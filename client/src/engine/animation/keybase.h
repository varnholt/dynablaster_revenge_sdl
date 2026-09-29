// base class for a key-frame, contains the non-template part of "key" (a timestamp)

#pragma once

#include <cstdint>
#include "tools/streamable.h"

class KeyBase : public Streamable
{
public:
   KeyBase(int32_t time = 0);
   virtual ~KeyBase() = default;

   int32_t time() const;

   void load(Stream* stream) override;
   void write(Stream* stream) override;

protected:
   int32_t _time = 0;
};
