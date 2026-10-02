#pragma once

#include <cstdint>
#include <string>

/// \brief cycles through the characters a gamepad can type into a line edit
class CharCycling
{
public:
   void up();
   void down();
   void reset();

   /// \brief continue cycling from the given character (case-insensitive)
   void setChar(char c);
   char getChar() const;

   /// \brief replaces the character at the cursor with the current one
   std::string modify(const std::string& input, int32_t cursor) const;

private:
   static constexpr const char* _chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890.:_-";
   int32_t _char_index = 0;
};
