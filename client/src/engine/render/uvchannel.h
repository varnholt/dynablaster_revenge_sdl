#pragma once

#include "tools/list.h"
#include "tools/stream.h"
#include "uv.h"

#include <cstdint>

class UVChannel : public Streamable
{
public:
   UVChannel() = default;
   UVChannel(int32_t id, UV* uv, int32_t size);
   virtual ~UVChannel() = default;

   void load(Stream* stream) override;
   void write(Stream* stream) override;
   void copy(const UVChannel& other);
   const List<UV>& getUV() const;

   int32_t id() const;

   UV* data() const;

private:
   int32_t _id = -1;
   List<UV> _uv;
};
