#include "random.h"

#include <random>

namespace Random
{
namespace
{
std::mt19937& engine()
{
   static std::mt19937 generator{std::random_device{}()};
   return generator;
}
}  // namespace

int bounded(int bound)
{
   if (bound <= 0)
   {
      return 0;
   }

   std::uniform_int_distribution<int> distribution(0, bound - 1);
   return distribution(engine());
}
}  // namespace Random
