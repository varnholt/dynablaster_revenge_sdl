#pragma once

#include <cstdint>

// loads "<fname>.tga" into a new[]-allocated 32bit pixel buffer returned in "buf" (a 1x1 white
// pixel if the file is missing); returns 32 on success, 0 on failure
int32_t loadtga(const char* fname, void** buf, int32_t* sizex, int32_t* sizey);
int32_t savetga(const char* fname, uint32_t* data, int32_t width, int32_t height);
