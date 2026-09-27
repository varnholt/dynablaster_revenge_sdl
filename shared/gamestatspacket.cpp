// header
#include "gamestatspacket.h"

// Qt
#include "logging.h"

// defines
#define PACKETNAME "GameStats"

//----------------------------------------------------------------------------
/*!
   \param message message to send
   \param receiverId id of the receiver
*/
GameStatsPacket::GameStatsPacket(
   const std::vector<int>& ids, const std::vector<PlayerStats>& overallStats, const std::vector<PlayerStats>& roundStats
)
    : Packet(Packet::GAMESTATS), mPlayerIds(ids), mOverallStats(overallStats), mRoundStats(roundStats)
{
   mPacketName = PACKETNAME;
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
   return mOverallStats;
}

//----------------------------------------------------------------------------
/*!
   \return round game stats
*/
std::vector<PlayerStats> GameStatsPacket::getRoundStats() const
{
   return mRoundStats;
}

//----------------------------------------------------------------------------
/*!
   \return player ids
*/
std::vector<int> GameStatsPacket::getPlayerIds() const
{
   return mPlayerIds;
}

//----------------------------------------------------------------------------
/*!
   \param out datastream to write members to
*/
void GameStatsPacket::enqueue(BinaryWriter& out)
{
   // explicit int32_t cast: QList::size() returns qsizetype (8 bytes) in Qt6, but dequeue()
   // below reads it back as a plain int (4 bytes) - a mismatch here desyncs the packet stream
   // for every packet after this one.
   out << (int32_t)mOverallStats.size();

   // write list of ids
   for (int id : mPlayerIds)
   {
      out << id;
   }

   std::vector<std::vector<PlayerStats>*> statLists{&mOverallStats, &mRoundStats};

   // write overall stats
   for (std::vector<PlayerStats>* list : statLists)
   {
      for (const PlayerStats& stats : *list)
      {
         out << stats.getWins() << stats.getKills() << stats.getDeaths() << stats.getSurvivalTime() << stats.getExtrasCollected();
      }
   }
}

//----------------------------------------------------------------------------
/*!
   \param in datastream read members from
   \param size list size
   \param list list to fill with values
*/
void GameStatsPacket::dequeueStatsList(BinaryReader& in, int size, std::vector<PlayerStats>* list)
{
   unsigned int wins = 0;
   unsigned int kills = 0;
   unsigned int deaths = 0;
   unsigned int survivalTime = 0;
   unsigned int extrasCollected = 0;

   for (int i = 0; i < size; i++)
   {
      PlayerStats stats;

      in >> wins >> kills >> deaths >> survivalTime >> extrasCollected;

      stats.setWins(wins);
      stats.setKills(kills);
      stats.setDeaths(deaths);
      stats.setSurvivalTime(survivalTime);
      stats.setExtrasCollected(extrasCollected);

      list->push_back(stats);
   }
}

//----------------------------------------------------------------------------
/*!
   \param in datastream read members from
*/
void GameStatsPacket::dequeue(BinaryReader& in)
{
   // read size
   int size = 0;
   in >> size;

   // read list of ids
   int id = 0;
   for (int i = 0; i < size; i++)
   {
      in >> id;
      mPlayerIds.push_back(id);
   }

   // read list of stats
   dequeueStatsList(in, size, &mOverallStats);
   dequeueStatsList(in, size, &mRoundStats);
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
