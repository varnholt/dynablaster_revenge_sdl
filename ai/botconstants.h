#ifndef BOTCONSTANTS_H
#define BOTCONSTANTS_H

class BotConstants
{
};

// it is import to have the keyboard correction epsilon smaller than the
// "field reached precision" -> so the bot doesn't think he missed a field
// after the keyboard correction was applied
inline constexpr float FIELD_REACHED_PRECISION = 0.15f;
inline constexpr float KEYBOARD_CORRECTION_EPSILON = 0.1f;

inline constexpr bool DEBUG_MAPITEMS = false;
inline constexpr bool DEBUG_PATH = false;
inline constexpr bool DEBUG_SCORES = false;
inline constexpr bool DEBUG_KEYSPRESSED = false;
inline constexpr bool DEBUG_POSSIBLE_ACTIONS = false;
inline constexpr bool DEBUG_EXECUTED_ACTIONS = false;
inline constexpr bool DEBUG_CURRENT_HAZARDOUS = false;
inline constexpr bool DEBUG_ESCAPE_PATH = false;
inline constexpr bool DEBUG_WALK_ACTION = false;
inline constexpr bool DEBUG_BOMB_DROP = false;

#endif  // BOTCONSTANTS_H
