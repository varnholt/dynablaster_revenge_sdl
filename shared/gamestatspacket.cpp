// header
#include "gamestatspacket.h"

// Qt
#include "logging.h"

#include <ranges>

// defines
#define PACKETNAME "GameStats"

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

//----------------------------------------------------------------------------
/*!
   \param message message to send
   \param receiverId id of the receiver
*/
GameStatsPacket::GameStatsPacket(
   const std::vector<int32_t>& ids, const std::vector<PlayerStats>& overallStats, const std::vector<PlayerStats>& roundStats
)
    : Packet(Packet::GAMESTATS)
{
   mPacketName = PACKETNAME;

   mPlayerStats.reserve(ids.size());
   for (const auto& [id, overall, round] : std::views::zip(ids, overallStats, roundStats))
   {
      mPlayerStats.push_back(PlayerGameStats{id, overall, round});
   }
}

//----------------------------------------------------------------------------
/*!
 */
GameStatsPacket::GameStatsPacket() : Packet(Packet::GAMESTATS)
{
   mPacketName = PACKETNAME;
}

//----------------------------------------------------------------------------
/*!
 */
GameStatsPacket::~GameStatsPacket()
{
}

//----------------------------------------------------------------------------
/*!
   \return overall game stats
*/
std::vector<PlayerStats> GameStatsPacket::getOverallStats() const
{
   return mPlayerStats | std::views::transform([](const PlayerGameStats& stats) { return stats.overall_stats; }) |
          std::ranges::to<std::vector>();
}

//----------------------------------------------------------------------------
/*!
   \return round game stats
*/
std::vector<PlayerStats> GameStatsPacket::getRoundStats() const
{
   return mPlayerStats | std::views::transform([](const PlayerGameStats& stats) { return stats.round_stats; }) |
          std::ranges::to<std::vector>();
}

//----------------------------------------------------------------------------
/*!
   \return player ids
*/
std::vector<int32_t> GameStatsPacket::getPlayerIds() const
{
   return mPlayerStats | std::views::transform([](const PlayerGameStats& stats) { return stats.player_id; }) |
          std::ranges::to<std::vector>();
}

//----------------------------------------------------------------------------
/*!
   \param out datastream to write members to
*/
void GameStatsPacket::enqueue(BinaryWriter& out)
{
   out << mPlayerStats;
}

//----------------------------------------------------------------------------
/*!
   \param in datastream read members from
*/
void GameStatsPacket::dequeue(BinaryReader& in)
{
   in >> mPlayerStats;
}

//----------------------------------------------------------------------------
/*!
   debug output of members
*/
void GameStatsPacket::debug()
{
   // debug message request
   qDebug("GameStatsPacket:debug: ");
}
