#pragma once

#include "bottype.h"
#include "playerstats.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

class Game;

// records what every player achieved in each round of a game, from the stats the server keeps
class SimRecorder
{
public:
   struct Row
   {
      int32_t _round = 0;
      int32_t _player_id = 0;
      std::string _nick;
      BotType _bot_type = BotType::Proto;
      uint32_t _kills = 0;
      uint32_t _deaths = 0;
      uint32_t _wins = 0;
      uint32_t _extras = 0;
      uint32_t _survival_s = 0;
   };

   //! starts listening to the game, which must outlive the recorder
   SimRecorder(Game& game, int32_t rounds);
   ~SimRecorder();

   SimRecorder(const SimRecorder&) = delete;
   SimRecorder& operator=(const SimRecorder&) = delete;

   //! the player is played by this bot type, players default to ProtoBot
   void setBotType(int32_t player_id, BotType type);

   //! all requested rounds were played
   [[nodiscard]] bool isFinished() const;

   [[nodiscard]] int32_t getRoundsPlayed() const;

   [[nodiscard]] const std::vector<Row>& getRows() const;

   //! writes the rows as csv, false if the file can't be written
   [[nodiscard]] bool write(const std::string& path) const;

   //! per player totals over all rounds, one line each
   [[nodiscard]] std::string summary() const;

private:
   //! a round ended, the difference to the last snapshot is what it brought
   void recordRound();

   Game& _game;
   int32_t _rounds = 0;
   int32_t _rounds_played = 0;
   std::size_t _state_connection = 0;
   std::map<int32_t, PlayerStats> _previous;
   std::map<int32_t, BotType> _bot_types;
   std::vector<Row> _rows;
};
