// vertex-indices (3 per polygon), streamed as an int32 count followed by 16 bit words

#pragma once

#include <cstdint>
#include <vector>

class Stream;

void loadFaceList(Stream& stream, std::vector<uint16_t>& indices);
void writeFaceList(Stream& stream, const std::vector<uint16_t>& indices);
