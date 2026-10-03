#include "facelist.h"
#include "tools/stream.h"

void loadFaceList(Stream& stream, std::vector<uint16_t>& indices)
{
   const int32_t size = stream.getInt();
   indices.resize(static_cast<size_t>(size));

   for (auto& index : indices)
   {
      index = static_cast<uint16_t>(stream.getWord());
   }
}

void writeFaceList(Stream& stream, const std::vector<uint16_t>& indices)
{
   stream.writeInt(static_cast<int32_t>(indices.size()));

   for (const auto index : indices)
   {
      stream.writeWord(index);
   }
}
