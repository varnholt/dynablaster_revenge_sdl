#pragma once

#include <cstdint>

namespace Random
{
//! returns a uniformly distributed random integer in [0, bound)
[[nodiscard]] int32_t bounded(int32_t bound);
}  // namespace Random
