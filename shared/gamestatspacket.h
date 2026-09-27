#pragma once

#include <cstdint>
#include <vector>

#include "packet.h"

#include "playerstats.h"

// one player's slice of a GameStatsPacket - id plus both stat sets, kept together so
// enqueue()/dequeue() can stream a single std::vector<PlayerGameStats> through BinaryWriter/
// BinaryReader's generic vector operator<</>> instead of the old three-parallel-vectors-plus-
// one-shared-size-field layout.
struct PlayerGameStats
{
   int32_t player_id = 0;
   PlayerStats overall_stats;
   PlayerStats round_stats;
};

BinaryWriter& operator<<(BinaryWriter& out, const PlayerGameStats& stats);
BinaryReader& operator>>(BinaryReader& in, PlayerGameStats& stats);

class GameStatsPacket : public Packet
{
public:
   //! write constructor
   GameStatsPacket(
      const std::vector<int32_t>& ids, const std::vector<PlayerStats>& overallStats, const std::vector<PlayerStats>& roundStats
   );

   //! read constructor
   GameStatsPacket();

   //! destructor
   virtual ~GameStatsPacket();

   //! debugs the member variables
   void debug();

   //! enqueues the member variables to datastream
   void enqueue(BinaryWriter&);

   //! dequeues the member variables from datastream
   void dequeue(BinaryReader&);

   //! getter for the overall player stats
   [[nodiscard]] std::vector<PlayerStats> getOverallStats() const;

   //! getter for the round player stats
   [[nodiscard]] std::vector<PlayerStats> getRoundStats() const;

   //! getter for list of player ids
   [[nodiscard]] std::vector<int32_t> getPlayerIds() const;

private:
   //! per-player id + overall/round stats, one entry per player
   std::vector<PlayerGameStats> mPlayerStats;
};
