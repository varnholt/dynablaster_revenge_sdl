#include "gamestatspacket.h"

#include "logging.h"

#include <string_view>

#include <ranges>

namespace
{
constexpr std::string_view PACKETNAME = "GameStats";
}

BinaryWriter& operator<<(BinaryWriter& out, const PlayerGameStats& stats)
{
   out << stats.player_id << stats.overall_stats << stats.round_stats;
   return out;
}

BinaryReader& operator>>(BinaryReader& in, PlayerGameStats& stats)
{
   in >> stats.player_id >> stats.overall_stats >> stats.round_stats;
   return in;
}

GameStatsPacket::GameStatsPacket(
   const std::vector<int32_t>& ids,
   const std::vector<PlayerStats>& overall_stats,
   const std::vector<PlayerStats>& round_stats
)
    : Packet(Packet::GAMESTATS)
{
   _packet_name = PACKETNAME;

   _player_stats.reserve(ids.size());
   for (const auto& [id, overall, round] : std::views::zip(ids, overall_stats, round_stats))
   {
      _player_stats.push_back(PlayerGameStats{id, overall, round});
   }
}

GameStatsPacket::GameStatsPacket() : Packet(Packet::GAMESTATS)
{
   _packet_name = PACKETNAME;
}

std::vector<PlayerStats> GameStatsPacket::getOverallStats() const
{
   auto view = _player_stats | std::views::transform([](const PlayerGameStats& stats) { return stats.overall_stats; });
   return {view.begin(), view.end()};
}

std::vector<PlayerStats> GameStatsPacket::getRoundStats() const
{
   auto view = _player_stats | std::views::transform([](const PlayerGameStats& stats) { return stats.round_stats; });
   return {view.begin(), view.end()};
}

std::vector<int32_t> GameStatsPacket::getPlayerIds() const
{
   auto view = _player_stats | std::views::transform([](const PlayerGameStats& stats) { return stats.player_id; });
   return {view.begin(), view.end()};
}

void GameStatsPacket::enqueue(BinaryWriter& out)
{
   out << _player_stats;
}

void GameStatsPacket::dequeue(BinaryReader& in)
{
   in >> _player_stats;
}

void GameStatsPacket::debug()
{
   qDebug("GameStatsPacket:debug: ");
}
