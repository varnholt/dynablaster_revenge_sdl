#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

//! the original game's eight letter level code: the stage and the abilities the player starts it with
//!
//! the format is the one of the DOS release (DYNA.EXE), so codes written down in 1991 still work and
//! codes shown by this game work in the original:
//! - the letters come in four pairs, every pair is one byte (low nibble first), each pair has its own
//!   alphabet of 16 letters
//! - the first byte holds a shift (bits 0..2), the marker 0b10 (bits 3..4) and an inversion mask (bits 5..7)
//! - the other three bytes hold, bit by bit scattered over them: world, stage, bombs, flames, the speed-up
//!   and a checksum (the number of bits set before it)
struct StoryCode
{
   //! 0..63, world * 8 + stage
   int32_t _stage_index = 0;

   //! 0..15 each, a code can't hold more
   int32_t _bombs = 1;
   int32_t _flames = 1;

   //! the original knows just one speed-up
   bool _speedup = false;

   //! the code's spelling; the original picks both at random, so one state has many codes
   //! shift 0..1 (the original's range, larger values lose bits), invert 0..7
   int32_t _shift = 0;
   int32_t _invert = 0;
};

//! the code for the given state, upper case
[[nodiscard]] std::string encodeStoryCode(const StoryCode& code);

//! the state behind a code, case insensitive; accepts what the original accepts: it only checks the
//! marker, the stage range and the checksum's top bit
[[nodiscard]] std::optional<StoryCode> decodeStoryCode(std::string_view text);
