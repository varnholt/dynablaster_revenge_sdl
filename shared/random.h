#pragma once

#include <cstdint>

namespace Random
{
//! returns a uniformly distributed random integer in [0, bound)
[[nodiscard]] int32_t bounded(int32_t bound);

//! restart the sequence from a fixed seed, so a headless simulation can be replayed
void seed(uint32_t value);
}  // namespace Random
