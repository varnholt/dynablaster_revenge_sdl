#include "storycode.h"

#include <algorithm>
#include <array>
#include <cctype>

namespace
{
// one alphabet per letter pair, DS:0x20cf in DYNA.EXE
constexpr std::array<std::string_view, 4> ALPHABETS = {
   "MUROYAGCQXSKIWHB",
   "WRECKFVBYHOZNAGL",
   "NEWSLOGIMPHJTVBQ",
   "NUCLEARHPSTVZYGK",
};

// where every bit lives: byte 1..3 of the code and the mask in it, most significant bit of a field first
// (DS:0x20b9 and DS:0x20a3)
struct BitLocation
{
   uint8_t _byte;
   uint8_t _mask;
};

constexpr std::array<BitLocation, 22> BITS = {{
   // world
   {1, 0x08}, {3, 0x04}, {2, 0x10}, {2, 0x04},
   // stage
   {3, 0x40}, {2, 0x40}, {1, 0x01}, {3, 0x08},
   // bombs
   {2, 0x80}, {1, 0x10}, {2, 0x01}, {3, 0x20},
   // flames
   {3, 0x10}, {2, 0x08}, {3, 0x02}, {1, 0x02},
   // speed-up
   {1, 0x20},
   // checksum
   {2, 0x02}, {3, 0x01}, {1, 0x04}, {3, 0x80}, {2, 0x20},
}};

struct Field
{
   size_t _first;
   size_t _count;
};

constexpr Field WORLD{0, 4};
constexpr Field STAGE{4, 4};
constexpr Field BOMBS{8, 4};
constexpr Field FLAMES{12, 4};
constexpr Field SPEEDUP{16, 1};
constexpr Field CHECKSUM{17, 5};

constexpr uint8_t MARKER_MASK = 0x18;
constexpr uint8_t MARKER = 0x10;

using Bytes = std::array<uint8_t, 4>;

// bytes 1..3 are one 24 bit value; bits pushed out at the top are lost, as in the original
void shiftLeft(Bytes& bytes, int32_t count)
{
   uint32_t value = bytes[1] | (bytes[2] << 8) | (bytes[3] << 16);
   value = (value << count) & 0xffffff;
   bytes[1] = static_cast<uint8_t>(value);
   bytes[2] = static_cast<uint8_t>(value >> 8);
   bytes[3] = static_cast<uint8_t>(value >> 16);
}

void shiftRight(Bytes& bytes, int32_t count)
{
   uint32_t value = bytes[1] | (bytes[2] << 8) | (bytes[3] << 16);
   value >>= count;
   bytes[1] = static_cast<uint8_t>(value);
   bytes[2] = static_cast<uint8_t>(value >> 8);
   bytes[3] = static_cast<uint8_t>(value >> 16);
}

void invert(Bytes& bytes, int32_t mask)
{
   for (size_t i = 0; i < 3; i++)
   {
      if ((mask >> i) & 1)
      {
         bytes[i + 1] = static_cast<uint8_t>(~bytes[i + 1]);
      }
   }
}

// returns the number of bits set
int32_t write(Bytes& bytes, Field field, int32_t value)
{
   int32_t ones = 0;
   for (size_t i = 0; i < field._count; i++)
   {
      if ((value >> (field._count - 1 - i)) & 1)
      {
         const BitLocation& bit = BITS[field._first + i];
         bytes[bit._byte] |= bit._mask;
         ones++;
      }
   }
   return ones;
}

int32_t read(const Bytes& bytes, Field field)
{
   int32_t value = 0;
   for (size_t i = 0; i < field._count; i++)
   {
      const BitLocation& bit = BITS[field._first + i];
      value = (value << 1) | ((bytes[bit._byte] & bit._mask) ? 1 : 0);
   }
   return value;
}
}  // namespace

std::string encodeStoryCode(const StoryCode& code)
{
   Bytes bytes{};

   int32_t ones = 0;
   ones += write(bytes, WORLD, code._stage_index / 8 + 1);
   ones += write(bytes, STAGE, code._stage_index % 8 + 1);
   ones += write(bytes, BOMBS, std::clamp(code._bombs, 0, 15));
   ones += write(bytes, FLAMES, std::clamp(code._flames, 0, 15));
   ones += write(bytes, SPEEDUP, code._speedup ? 1 : 0);
   write(bytes, CHECKSUM, ones);

   const int32_t shift = std::clamp(code._shift, 0, 7);
   const int32_t mask = code._invert & 7;
   shiftLeft(bytes, shift);
   bytes[0] = static_cast<uint8_t>(shift | MARKER | (mask << 5));
   invert(bytes, mask);

   std::string text;
   for (size_t i = 0; i < bytes.size(); i++)
   {
      text += ALPHABETS[i][bytes[i] & 0xf];
      text += ALPHABETS[i][bytes[i] >> 4];
   }
   return text;
}

std::optional<StoryCode> decodeStoryCode(std::string_view text)
{
   if (text.size() != 8)
   {
      return std::nullopt;
   }

   Bytes bytes{};
   for (size_t i = 0; i < bytes.size(); i++)
   {
      const auto low = ALPHABETS[i].find(static_cast<char>(std::toupper(static_cast<unsigned char>(text[i * 2]))));
      const auto high = ALPHABETS[i].find(static_cast<char>(std::toupper(static_cast<unsigned char>(text[i * 2 + 1]))));
      if (low == std::string_view::npos || high == std::string_view::npos)
      {
         return std::nullopt;
      }
      bytes[i] = static_cast<uint8_t>(low | (high << 4));
   }

   if ((bytes[0] & MARKER_MASK) != MARKER)
   {
      return std::nullopt;
   }

   StoryCode code;
   code._shift = bytes[0] & 7;
   code._invert = bytes[0] >> 5;
   invert(bytes, code._invert);
   shiftRight(bytes, code._shift);

   const int32_t world = read(bytes, WORLD);
   const int32_t stage = read(bytes, STAGE);
   if (world < 1 || world > 8 || stage < 1 || stage > 8)
   {
      return std::nullopt;
   }

   // the original only looks at the checksum's top bit; with shift 1 its own encoder pushes another
   // checksum bit out of the code, so checking more would reject genuine codes
   if (read(bytes, CHECKSUM) >> 4)
   {
      return std::nullopt;
   }

   code._stage_index = (world - 1) * 8 + (stage - 1);
   code._bombs = read(bytes, BOMBS);
   code._flames = read(bytes, FLAMES);
   code._speedup = read(bytes, SPEEDUP) != 0;
   return code;
}
