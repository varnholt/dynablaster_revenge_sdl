#include "simrecorder.h"

#include "game.h"
#include "player.h"

#include <format>
#include <fstream>

SimRecorder::SimRecorder(Game& game, int32_t rounds) : _game(game), _rounds(rounds)
{
   _state_connection = _game.stateChangedSignal.connect(
      [this](Constants::GameState state)
      {
         if (state == Constants::GameStopped)
         {
            recordRound();
         }
      }
   );
}

SimRecorder::~SimRecorder()
{
   _game.stateChangedSignal.disconnect(_state_connection);
}

void SimRecorder::setBotType(int32_t player_id, BotType type)
{
   _bot_types[player_id] = type;
}

bool SimRecorder::isFinished() const
{
   return _rounds_played >= _rounds;
}

int32_t SimRecorder::getRoundsPlayed() const
{
   return _rounds_played;
}

const std::vector<SimRecorder::Row>& SimRecorder::getRows() const
{
   return _rows;
}

void SimRecorder::recordRound()
{
   _rounds_played++;

   for (Player& player : _game.getPlayers())
   {
      const PlayerStats& now = player.getOverallStats();
      const PlayerStats& before = _previous[player.getId()];

      Row row;
      row._round = _rounds_played;
      row._player_id = player.getId();
      row._nick = player.getNick();
      row._bot_type = _bot_types.contains(player.getId()) ? _bot_types.at(player.getId()) : BotType::Proto;
      row._kills = now.getKills() - before.getKills();
      row._deaths = now.getDeaths() - before.getDeaths();
      row._wins = now.getWins() - before.getWins();
      row._extras = now.getExtrasCollected() - before.getExtrasCollected();
      row._survival_s = now.getSurvivalTime() - before.getSurvivalTime();
      _rows.push_back(row);

      _previous[player.getId()] = now;
   }
}

bool SimRecorder::write(const std::string& path) const
{
   std::ofstream out(path);

   if (!out)
   {
      return false;
   }

   out << "round,player,nick,bot,kills,deaths,wins,extras,survival_s\n";

   for (const Row& row : _rows)
   {
      out << std::format(
         "{},{},{},{},{},{},{},{},{}\n",
         row._round,
         row._player_id,
         row._nick,
         toString(row._bot_type),
         row._kills,
         row._deaths,
         row._wins,
         row._extras,
         row._survival_s
      );
   }

   return static_cast<bool>(out);
}

std::string SimRecorder::summary() const
{
   struct Total
   {
      std::string _nick;
      BotType _bot_type = BotType::Proto;
      uint32_t _kills = 0;
      uint32_t _deaths = 0;
      uint32_t _wins = 0;
      uint32_t _extras = 0;
   };

   std::map<int32_t, Total> totals;

   for (const Row& row : _rows)
   {
      Total& total = totals[row._player_id];
      total._nick = row._nick;
      total._bot_type = row._bot_type;
      total._kills += row._kills;
      total._deaths += row._deaths;
      total._wins += row._wins;
      total._extras += row._extras;
   }

   std::string text;

   for (const auto& [id, total] : totals)
   {
      text += std::format(
         "player {} {:<10} {:<7} wins {:>3}  kills {:>3}  deaths {:>3}  extras {:>3}\n",
         id,
         total._nick,
         toString(total._bot_type),
         total._wins,
         total._kills,
         total._deaths,
         total._extras
      );
   }

   return text;
}
