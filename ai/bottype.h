#ifndef BOTTYPE_H
#define BOTTYPE_H

#include <optional>
#include <string_view>

//! which bot plays a computer player
enum class BotType
{
   Proto,   //!< the classic bot with its fixed priorities, the default
   Learned  //!< ProtoBot's safety and walking, goals chosen by weights trained in dynablaster_sim
};

//! name used in game.ini and on the sim's command line: "proto" or "learned"
std::string_view toString(BotType type);

//! parses a name written by toString()
std::optional<BotType> botTypeFromString(std::string_view name);

#endif  // BOTTYPE_H
