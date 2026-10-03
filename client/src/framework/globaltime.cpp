#include "globaltime.h"

#include <functional>
#include <optional>

namespace
{
std::optional<std::reference_wrapper<GlobalTime>> instance;
}  // namespace

GlobalTime::GlobalTime()
{
   instance = *this;
}

GlobalTime::~GlobalTime()
{
   instance.reset();
}

GlobalTime& GlobalTime::Instance()
{
   return instance.value();
}
