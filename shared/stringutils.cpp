#include "stringutils.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <ranges>
#include <sstream>

namespace StringUtils
{
std::string toLower(const std::string& str)
{
   std::string result = str;
   std::ranges::transform(result, result.begin(), [](uint8_t c) { return std::tolower(c); });
   return result;
}

std::string toUpper(const std::string& str)
{
   std::string result = str;
   std::ranges::transform(result, result.begin(), [](uint8_t c) { return std::toupper(c); });
   return result;
}

std::string trim(const std::string& str)
{
   const auto is_whitespace = [](uint8_t character) { return std::isspace(character) != 0; };

   const auto first = std::ranges::find_if_not(str, is_whitespace);
   const auto last = std::ranges::find_if_not(str | std::views::reverse, is_whitespace).base();

   if (first >= last)
   {
      return {};
   }

   return std::string{first, last};
}

std::vector<std::string> split(const std::string& str, char separator)
{
   std::vector<std::string> parts;
   std::istringstream stream{str};
   std::string part;

   while (std::getline(stream, part, separator))
   {
      if (!part.empty())
      {
         parts.push_back(part);
      }
   }

   return parts;
}
}  // namespace StringUtils
