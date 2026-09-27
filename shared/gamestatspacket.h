#ifndef GAMESTATSPACKET_H
#define GAMESTATSPACKET_H

#include <vector>

#include "packet.h"

#include "playerstats.h"

class GameStatsPacket : public Packet
{
public:
   //! write constructor
   GameStatsPacket(
      const std::vector<int>& ids, const std::vector<PlayerStats>& overallStats, const std::vector<PlayerStats>& roundStats
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
   std::vector<PlayerStats> getOverallStats() const;

   //! getter for the round player stats
   std::vector<PlayerStats> getRoundStats() const;

   //! getter for list of player ids
   std::vector<int> getPlayerIds() const;

private:
   //! dequeue stats list
   void dequeueStatsList(BinaryReader& in, int size, std::vector<PlayerStats>* list);

   //! list of player ids
   std::vector<int> mPlayerIds;

   //! player stats to send
   std::vector<PlayerStats> mOverallStats;

   //! player rounds stats to send
   std::vector<PlayerStats> mRoundStats;
};

#endif  // GAMESTATSPACKET_H
