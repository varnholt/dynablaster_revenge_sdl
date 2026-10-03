#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

// loads "<fname>.tga" into a 32bit pixel buffer (a 1x1 white pixel if the file is missing);
// returns 32 on success, 0 on failure
int32_t loadtga(const std::string& fname, std::vector<uint32_t>& pixels, int32_t& width, int32_t& height);
int32_t savetga(const std::string& fname, std::span<const uint32_t> data, int32_t width, int32_t height);
