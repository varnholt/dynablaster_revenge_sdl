// level codes against the ones the DOS release of the original game produced (DYNA.COD as shipped
// with it, decoded by a separate reimplementation); exits with 1 on the first mismatch
#include "storycode.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

namespace
{
struct Vector
{
   std::string_view _code;
   int _stage_index;
   int _bombs;
   int _flames;
   bool _speedup;
};

// two of DYNA.COD's 64 lines are off: MAVEBHCP is listed as 2-3 but is a 2-4 code the game would
// produce, MKAAOEYS (3-8) has a typo the game tolerates, its encoder would write MKAAOEZS
constexpr Vector ORIGINAL_CODES[] = {
      {"MBHLJQHY", 0, 1, 1, false},
      {"MUFWLNCC", 1, 1, 2, false},
      {"MOOLOWTN", 2, 2, 2, false},
      {"MAKWHJCN", 3, 2, 2, false},
      {"UUYWHTEU", 4, 2, 2, false},
      {"MCOLHPCN", 5, 2, 2, false},
      {"MBOLHJAH", 6, 2, 2, false},
      {"MOZLONCE", 7, 2, 2, false},
      {"UKBLWWVG", 8, 2, 2, false},
      {"MXBWESYK", 9, 2, 3, false},
      {"MAVEBHCP", 11, 2, 3, true},
      {"MBHABHYH", 11, 2, 3, true},
      {"MOHAEITP", 12, 2, 3, true},
      {"MBYABMYH", 13, 2, 3, true},
      {"MUCEEOVN", 14, 2, 3, true},
      {"MCHABBCZ", 15, 2, 3, true},
      {"UKCZHGVG", 16, 2, 3, true},
      {"UBRZOPVK", 17, 2, 3, true},
      {"MWCEHTEY", 18, 3, 3, true},
      {"MAEEHHLC", 19, 3, 3, true},
      {"MKAAOIEY", 20, 3, 3, true},
      {"UUVKHBRE", 21, 3, 3, true},
      {"UKHZHHST", 22, 3, 3, true},
      {"MKAAOEYS", 23, 3, 3, true},
      {"UANKVJZA", 24, 3, 3, true},
      {"UXGVNNLK", 25, 4, 3, true},
      {"MUBCNWGP", 26, 4, 3, true},
      {"MWVCQJSH", 27, 4, 3, true},
      {"UXNVNTLG", 28, 4, 3, true},
      {"MBONIJVH", 29, 4, 4, true},
      {"UXOVNVHG", 30, 4, 4, true},
      {"UXYVNEHL", 31, 5, 4, true},
      {"MBZNSVLA", 32, 5, 4, true},
      {"MAFCSVET", 33, 5, 4, true},
      {"UWVVIHAT", 34, 5, 5, true},
      {"MOANTLAC", 35, 5, 5, true},
      {"UOZHHVTU", 36, 6, 5, true},
      {"UBHHOWAK", 37, 6, 5, true},
      {"UUVVHPTU", 38, 6, 5, true},
      {"UKZHHEAH", 39, 6, 5, true},
      {"MKANPECK", 40, 6, 5, true},
      {"UCHHVTTN", 41, 6, 5, true},
      {"UOHHWITU", 42, 6, 5, true},
      {"MXECPOTK", 43, 6, 5, true},
      {"MOANPOYT", 44, 7, 5, true},
      {"UKAHWJUV", 45, 7, 6, true},
      {"MWRCGMNA", 46, 7, 6, true},
      {"MAWCGTHR", 47, 7, 6, true},
      {"MOLNVEKT", 48, 7, 6, true},
      {"MWRESGPK", 49, 8, 6, true},
      {"MCGASLKN", 50, 8, 6, true},
      {"MOLATVHN", 51, 8, 6, true},
      {"UBLZINNG", 52, 8, 6, true},
      {"UUEKMQKN", 53, 8, 6, true},
      {"MARESWKP", 54, 8, 6, true},
      {"MXWETPPV", 55, 8, 6, true},
      {"MBCAIIAH", 56, 8, 6, true},
      {"UKFONETK", 57, 8, 6, true},
      {"MWZEIIEK", 58, 8, 7, true},
      {"MORAMBCP", 59, 8, 7, true},
      {"MWOEISEK", 60, 8, 7, true},
      {"UCHOQWHE", 61, 9, 7, true},
      {"MKKAMTEA", 62, 9, 7, true},
      {"UWKFQBPL", 63, 9, 7, true},
};

int failures = 0;

void check(bool ok, std::string_view what, std::string_view code)
{
   if (!ok)
   {
      std::printf("FAIL %.*s: %.*s\n", static_cast<int>(what.size()), what.data(), static_cast<int>(code.size()), code.data());
      failures++;
   }
}
}  // namespace

int main()
{
   for (const Vector& vector : ORIGINAL_CODES)
   {
      const auto decoded = decodeStoryCode(vector._code);
      check(decoded.has_value(), "rejected", vector._code);
      if (!decoded)
      {
         continue;
      }

      check(decoded->_stage_index == vector._stage_index, "stage", vector._code);
      check(decoded->_bombs == vector._bombs, "bombs", vector._code);
      check(decoded->_flames == vector._flames, "flames", vector._code);
      check(decoded->_speedup == vector._speedup, "speed-up", vector._code);

      // spelled the same when encoded again, except for the typo the original tolerates
      if (vector._code != "MKAAOEYS")
      {
         check(encodeStoryCode(*decoded) == vector._code, "re-encoded", vector._code);
      }
   }

   // case doesn't matter, garbage is rejected
   check(decodeStoryCode("mbhljqhy").has_value(), "lower case", "mbhljqhy");
   check(!decodeStoryCode("MBHLJQH").has_value(), "too short", "MBHLJQH");
   check(!decodeStoryCode("ZZZZZZZZ").has_value(), "letters", "ZZZZZZZZ");

   // every state survives a round trip in every spelling the original uses
   for (int stage_index = 0; stage_index < 64; stage_index++)
   {
      for (int bombs = 0; bombs < 16; bombs++)
      {
         for (int flames = 0; flames < 16; flames++)
         {
            for (int variant = 0; variant < 32; variant++)
            {
               StoryCode code;
               code._stage_index = stage_index;
               code._bombs = bombs;
               code._flames = flames;
               code._speedup = (variant & 1) != 0;
               code._shift = (variant >> 1) & 1;
               code._invert = variant >> 2;

               const std::string text = encodeStoryCode(code);
               const auto decoded = decodeStoryCode(text);
               const bool ok = decoded && decoded->_stage_index == stage_index && decoded->_bombs == bombs &&
                               decoded->_flames == flames && decoded->_speedup == code._speedup;
               check(ok, "round trip", text);
            }
         }
      }
   }

   std::printf("%s (%d failures)\n", failures ? "FAILED" : "passed", failures);
   return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
