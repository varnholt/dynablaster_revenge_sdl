#include "profiling.h"

#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
#endif

double getCpuTick()
{
   double tick = 0.0;

#ifdef _MSC_VER
#ifdef WIN32
   tick = static_cast<double>(__rdtsc());
#endif
#endif

#ifdef _LINUX_
   uint32_t low = 0;
   uint32_t high = 0;
   // read timestamp counter (64bit) into edx:eax
   asm volatile("rdtsc" : "=a"(low), "=d"(high));
   const int64_t count = static_cast<int64_t>(low) | (static_cast<int64_t>(high) << 32);
   tick = static_cast<double>(count);
#endif

   return tick;
}
