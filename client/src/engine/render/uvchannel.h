#pragma once

#include "tools/stream.h"
#include "tools/streamable.h"
#include "uv.h"

#include <cstdint>
#include <span>
#include <vector>

class UVChannel : public Streamable
{
public:
   UVChannel() = default;
   UVChannel(int32_t id, std::span<const UV> uv);
   virtual ~UVChannel() = default;

   void load(Stream& stream) override;
   void write(Stream& stream) override;
   const std::vector<UV>& getUV() const;
   std::vector<UV>& getUV();

   int32_t id() const;

   UV* data();

private:
   int32_t _id = -1;
   std::vector<UV> _uv;
};
