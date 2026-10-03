#pragma once

#include <cstdint>
#include <span>
#include <string>

int32_t saveBmp(const std::string& filename, std::span<const uint32_t> data, int32_t x, int32_t y);
