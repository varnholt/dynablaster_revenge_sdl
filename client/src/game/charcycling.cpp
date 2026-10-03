#include "charcycling.h"

#include <algorithm>
#include <cctype>

namespace
{
int32_t charCount(std::string_view chars)
{
   return static_cast<int32_t>(chars.size());
}
}  // namespace

void CharCycling::up()
{
   _char_index++;
}

void CharCycling::down()
{
   _char_index--;
   if (_char_index < 0)
   {
      _char_index = charCount(_chars) - 1;
   }
}

void CharCycling::reset()
{
   _char_index = 0;
}

void CharCycling::setChar(char c)
{
   const auto lower = std::tolower(static_cast<unsigned char>(c));
   for (int32_t i = 0; i < charCount(_chars); ++i)
   {
      if (std::tolower(static_cast<unsigned char>(_chars[i])) == lower)
      {
         _char_index = i;
         break;
      }
   }
}

char CharCycling::getChar() const
{
   return _chars[_char_index % charCount(_chars)];
}

std::string CharCycling::modify(const std::string& input, int32_t cursor) const
{
   const auto length = static_cast<int32_t>(input.size());
   std::string altered = input.substr(0, static_cast<size_t>(std::clamp(cursor, 0, length)));
   altered.push_back(getChar());
   if (cursor + 1 < length)
   {
      altered += input.substr(static_cast<size_t>(cursor + 1));
   }
   return altered;
}
