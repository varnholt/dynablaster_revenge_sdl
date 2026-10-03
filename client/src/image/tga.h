#pragma once

#include <cstdint>
#include <vector>

// loads "<fname>.tga" into a 32bit pixel buffer (a 1x1 white pixel if the file is missing);
// returns 32 on success, 0 on failure
int32_t loadtga(const char* fname, std::vector<uint32_t>& pixels, int32_t& width, int32_t& height);
int32_t savetga(const char* fname, uint32_t* data, int32_t width, int32_t height);
