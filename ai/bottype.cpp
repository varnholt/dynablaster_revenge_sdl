#include "bottype.h"

std::string_view toString(BotType type)
{
   return type == BotType::Learned ? "learned" : "proto";
}

std::optional<BotType> botTypeFromString(std::string_view name)
{
   if (name == "proto")
   {
      return BotType::Proto;
   }

   if (name == "learned")
   {
      return BotType::Learned;
   }

   return std::nullopt;
}
