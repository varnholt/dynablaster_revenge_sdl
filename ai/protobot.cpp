#include "protobot.h"

// astar
#include "astarmap.h"
#include "astarpathfinding.h"

// ai
#include "bombchainreaction.h"
#include "botbombaction.h"
#include "botbombmapitem.h"
#include "botcharacter.h"
#include "botconstants.h"
#include "botidleaction.h"
#include "botmap.h"
#include "botplayerinfo.h"
#include "botwalkaction.h"
#include "protobotinsults.h"
#include "protobotmemory.h"

// shared
#include "logging.h"
#include "mapitem.h"
#include "playerdisease.h"
#include "random.h"
#include "weighted.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <limits>
#include <string>

namespace
{
constexpr int MANHATTAN_LENGTH_MAX_EXTRAS = 8;
constexpr int MANHATTAN_LENGTH_MAX_ATTACK = 5;
constexpr int MANHATTAN_LENGTH_ESCAPE_NEAR_MAX = 5;
constexpr int MANHATTAN_LENGTH_ESCAPE_MEDIUM_MIN = 6;
constexpr int MANHATTAN_LENGTH_ESCAPE_MEDIUM_MAX = 10;

//! field position of a player
Point toField(float x, float y)
{
   return Point(static_cast<int32_t>(std::floor(x)), static_cast<int32_t>(std::floor(y)));
}
}  // namespace

ProtoBot::ProtoBot()
{
   _bot_character = std::make_unique<BotCharacter>();
   _bot_character->setCharacter(
      randomize(2, 4),  // extra (2 is minimum as the value is multiplied)
      randomize(2, 4),  // bomb stone
      10                // randomize(0, 4)   // attack
   );

   _directions = {Point(0, -1), Point(0, 1), Point(-1, 0), Point(1, 0)};

   _bomb_chain_reaction = std::make_unique<BombChainReaction>();
   _bomb_chain_reaction->initDirections();

   _memory = std::make_unique<ProtoBotMemory>();
}

ProtoBot::~ProtoBot() = default;

/*!
   \param player id
   \param x position
   \param y position
   \param angle player angle
*/
void ProtoBot::updatePlayerPosition(int id, float x, float y, float angle)
{
   Bot::updatePlayerPosition(id, x, y, angle);

   // these are for debugging purposes only
   // remove this code as soon we're cool.
   // updatePositionQueue();

   /*
   if (isPositionQueueRecurrent())
   {
      setDebugBreakpointEnabled(true);

      // setDebugEscapePathsEnabled(true);
      // setDebugBombDropEnabled(true);
      // setDebugCurrentHazardousEnabled(true);
      // setDebugExecutedActionsEnabled(true);
      // setDebugKeysPressedEnabled(true);
      // setDebugMapItemsEnabled(true);
      // setDebugPathsEnabled(true);
      // setDebugPossibleActionsEnabled(true);
      // setDebugScoresEnabled(true);
      // setDebugWalkActionEnabled(true);
   }
   */
}

/*!
   \param item_id item that contains an extra
*/
void ProtoBot::extraShake(int item_id)
{
   Bot::extraShake(item_id);

   _extra_shake_ids.push_back(item_id);
}

void ProtoBot::wakeUp()
{
   Bot::wakeUp();
}

void ProtoBot::decide()
{
   Bot::decide();
}

void ProtoBot::act()
{
   Bot::act();
}

void ProtoBot::reset()
{
   _extra_shake_ids.clear();
   _memory->reset();
   resetIdleCounter();
   resetHazardousTemporary();
   resetFieldBombTimes();
}

/*!
   \param random minimum
   \param random maximum
   \return randomized value
*/
int ProtoBot::randomize(int min, int max)
{
   return Random::bounded((max + 1) - min) + min;
}

/*!
   \param x x position
   \param y y position
   \return field score at x,y
*/
int ProtoBot::getRemainingBombTime(int x, int y) const
{
   return _field_bomb_times[y * _bot_map->getWidth() + x];
}

/*!
   \param x x position
   \param y y position
   \param score score to set
*/
void ProtoBot::setRemainingBombTime(int x, int y, int score)
{
   int current_score = getRemainingBombTime(x, y);
   _field_bomb_times[y * _bot_map->getWidth() + x] = std::min(current_score, score);
}

/*!
   \param x x position
   \param y y position
   \return field score at x,y
*/
int ProtoBot::getScore(int x, int y) const
{
   return _field_scores[y * _bot_map->getWidth() + x];
}

/*!
   \param x x position
   \param y y position
   \param score score to set
*/
void ProtoBot::setScore(int x, int y, int score)
{
   _field_scores[y * _bot_map->getWidth() + x] = score;
}

/*!
   \param x x position
   \param y y position
   \param factor factor to apply
*/
void ProtoBot::multiplyScore(int x, int y, int factor)
{
   if (factor != 0)
   {
      _field_scores[y * _bot_map->getWidth() + x] *= factor;
   }
}

/*!
   \param x x position
   \param y y position
   \return ms value
*/
int ProtoBot::getHazardousTemporary(int x, int y) const
{
   return _hazardous_temporary[y * _bot_map->getWidth() + x];
}

/*!
   \param x x position
   \param y y position
   \param ms millis
   \param field_count no of fields to mark
*/
void ProtoBot::markHazardousTemporary(int x, int y, int ms, int field_count)
{
   if (x >= 0 && x < _bot_map->getWidth() && y >= 0 && y < _bot_map->getHeight())
   {
      _hazardous_temporary[y * _bot_map->getWidth() + x] = ms;
   }

   if (field_count > 0)
   {
      for (int i = 1; i <= field_count; i++)
      {
         int left = x - i;
         int right = x + i;
         int up = y - i;
         int down = y + i;

         if (left >= 0)
         {
            markHazardousTemporary(left, y, ms);
         }

         if (right < _bot_map->getWidth())
         {
            markHazardousTemporary(right, y, ms);
         }

         if (up >= 0)
         {
            markHazardousTemporary(x, up, ms);
         }

         if (down < _bot_map->getHeight())
         {
            markHazardousTemporary(x, down, ms);
         }
      }
   }
}

/*!
   \param ms elapsed
*/
void ProtoBot::updateHazardousTemporary(int ms)
{
   int diff = 0;
   for (int y = 0; y < _bot_map->getHeight(); y++)
   {
      for (int x = 0; x < _bot_map->getWidth(); x++)
      {
         int value = getHazardousTemporary(x, y);

         if (value > 0)
         {
            diff = value - ms;
            markHazardousTemporary(x, y, std::max(0, diff));

            if (diff > 0)
            {
               setScore(x, y, -1);
            }
         }
      }
   }
}

void ProtoBot::resetHazardousTemporary()
{
   std::ranges::fill(_hazardous_temporary, 0);
}

void ProtoBot::resetFieldBombTimes()
{
   std::ranges::fill(_field_bomb_times, 0xFFFF);
}

void ProtoBot::bugTrack1()
{
   /*
      [S][S][S]
      [S][ ][S]
      [S][P][S]
      [S][ ][S]
      [S][S][S]

   p.x = 6
   p.y = 5

   => reachable position count = 3

   */

   if (getXField() == 6 && getYField() == 5)
   {
      if (_bot_map->getReachablePositions().size() == 3)
      {
         qDebug("ProtoBot::bugTrack1(): issue 1 spotted");
      }
   }
}

void ProtoBot::bugTrack2()
{
   if (_bot_map->getItem(getXField(), getYField()) && _bot_map->getItem(getXField(), getYField())->getType() == MapItem::Bomb &&
       getScore(getXField(), getYField()) == 1)
   {
      qDebug("ErrorConditionTest");
   }
}

void ProtoBot::bugTrack3()
{
   increaseIdleCounter();

   if (getIdleCounter() % 5)
   {
      Point p(getXField(), getYField());

      _last_positions.push_back(p);

      while (_last_positions.size() > 100)
      {
         _last_positions.pop_front();
      }

      if (_last_positions.size() > 90)
      {
         bool diff = true;
         Point prev = _last_positions.front();
         for (const Point& position : _last_positions)
         {
            if (prev.x() != position.x() || prev.y() != position.y())
            {
               diff = false;
            }

            prev = position;
         }

         if (diff)
         {
            qDebug("player is totally idle :(");
         }
      }
   }
}

/*!
   \return idle counter
*/
int ProtoBot::getIdleCounter() const
{
   return _idle;
}

/*!
  \param value idle counter
*/
void ProtoBot::setIdleCounter(int value)
{
   _idle = value;
}

void ProtoBot::resetIdleCounter()
{
   _idle = 0;
}

void ProtoBot::increaseIdleCounter()
{
   _idle++;
}

/*!
   \return \c true if we can escape safely
*/
bool ProtoBot::isSafeEscapePossible()
{
   std::vector<Point> reachable_positions_filtered =
      reachablePositionsLeft(getXField(), getYField(), _player_info->getFlameCount(), _bot_map->getReachablePositions());

   // optimization
   // filter list of points again by manhattan length
   reachable_positions_filtered =
      Map::getManhattanFiltered(Point(getXField(), getYField()), reachable_positions_filtered, MANHATTAN_LENGTH_MAX_ATTACK);

   if (reachable_positions_filtered.size() > 100)
   {
      qWarning("ProtoBot::isAttackPossible(): cpu usage exceeded");
   }

   bool found_safe_drop_position = false;
   for (const Point& potential_safe_point : reachable_positions_filtered)
   {
      // score path to bomb drop position
      findPath(potential_safe_point.x(), potential_safe_point.y());
      int path_length = _path_finding.getPathLength();

      // a safe position must not be our current position
      if (path_length > 0)
      {
         if (!isPathHazardous(_path_finding.getPath()))
         {
            found_safe_drop_position = true;
         }
      }

      clearPath();

      if (found_safe_drop_position)
      {
         break;
      }
   }

   return found_safe_drop_position;
}

/*!
   \return \c true if attack is a cool idea
*/
bool ProtoBot::isAttackPossible()
{
   if (isNoBombInfectionActive())
   {
      return false;
   }

   bool attack_possible = false;

   if (_bot_map->isBombAmountConsumed(_id, _player_info->getBombCount()))
   {
      bool bomb_drop_deadly = _bot_map->isBombDropDeadly(getXField(), getYField(), _player_info->getFlameCount(), _enemy_positions);

      bool found_safe_drop_position = isSafeEscapePossible();

      attack_possible = bomb_drop_deadly && found_safe_drop_position;
   }

   return attack_possible;
}

bool ProtoBot::isBombStonePossible()
{
   int x_field = getXField();
   int y_field = getYField();
   Point bomb_stone_position = getBombStonePosition();

   bool at_bomb_stone_position = (x_field == bomb_stone_position.x() && y_field == bomb_stone_position.y());

   /*
      goal: only place bombs in the X'ed areas of a field

      +---+-----------+---+
      |   |           |   |
      +---+-----------+---+
      |   |XXXXXXXXXXX|   |
      |   |XXXXXXXXXXX|   |
      |   |XXXXXXXXXXX|   |
      |   |XXXXXXXXXXX|   |
      |   |XXXXXXXXXXX|   |
      +---+-----------+---+
      |   |           |   |
      +---+-----------+---+

   */

   bool within_epsilon = false;

   if (at_bomb_stone_position)
   {
      float eps = 0.05f;

      float x = static_cast<float>(x_field);
      float y = static_cast<float>(y_field);

      within_epsilon = (getX() > (x + eps) && getX() < (x + 1.0f - eps)) && (getY() > (y + eps) && getY() < (y + 1.0f - eps));
   }

   bool escape_possible = isSafeEscapePossible();

   return at_bomb_stone_position && escape_possible && within_epsilon;
}

void ProtoBot::markReachableFields()
{
   setScore(getXField(), getYField(), 1);

   for (const Point& p : _bot_map->getReachablePositions())
   {
      setScore(p.x(), p.y(), 1);
   }
}

void ProtoBot::markHazardousFields()
{
   // don't
   std::vector<BotBombMapItem*> bombs = _bot_map->getBombs();

   int x = 0;
   int y = 0;
   int xi = 0;
   int yi = 0;

   for (BotBombMapItem* bomb : bombs)
   {
      x = bomb->getX();
      y = bomb->getY();

      setScore(x, y, -1);

      for (const Point& direction : _directions)
      {
         // players infected with "small bomb" disease could have
         // only one flame..
         //
         //         if (bomb->getFlames() < 2)
         //         {
         //            qWarning(
         //               "ProtoBot::markHazardousFields(): the bot is "
         //               "obviously fucked. that is too bad."
         //            );
         //         }

         for (int i = 1; i <= bomb->getFlames(); i++)
         {
            xi = x + i * direction.x();
            yi = y + i * direction.y();

            if (xi >= 0 && xi < _bot_map->getWidth() && yi >= 0 && yi < _bot_map->getHeight())
            {
               setScore(xi, yi, -1);

               // we hit something
               if (_bot_map->getItem(xi, yi))
               {
                  break;
               }
            }
         }
      }
   }
}

void ProtoBot::updateRemainingBombTimes()
{
   // init
   auto current_time = std::chrono::steady_clock::now();
   int tick_time = getServerConfiguration().getBombTickTime();
   int time_diff = 0;
   int time_left = 0;
   int x = 0;
   int y = 0;
   int xi = 0;
   int yi = 0;

   // clear remaining bomb time array
   resetFieldBombTimes();

   _bomb_chain_reaction->setBotMap(_bot_map);
   _bomb_chain_reaction->compute();

   for (const std::vector<BotBombMapItem*>& connected_bombs : _bomb_chain_reaction->getDetonationChain())
   {
      int min = 0xFFFF;

      for (BotBombMapItem* item : connected_bombs)
      {
         time_diff = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(current_time - item->getDropTime()).count());
         time_left = std::max(tick_time - time_diff, 0);

         min = std::min(min, time_left);
      }

      // now create a map of detonation times
      for (BotBombMapItem* item : connected_bombs)
      {
         x = item->getX();
         y = item->getY();

         setRemainingBombTime(x, y, min);

         for (const Point& direction : _directions)
         {
            for (int i = 1; i <= item->getFlames(); i++)
            {
               xi = x + i * direction.x();
               yi = y + i * direction.y();

               if (xi >= 0 && xi < _bot_map->getWidth() && yi >= 0 && yi < _bot_map->getHeight())
               {
                  setRemainingBombTime(xi, yi, min);

                  // we hit something
                  if (_bot_map->getItem(xi, yi))
                  {
                     break;
                  }
               }
               else
               {
                  // we hit a wall
                  break;
               }
            }
         }
      }
   }
}

void ProtoBot::resetScores()
{
   std::ranges::fill(_field_scores, 0);
}

/*!
   \return bomb stone position
*/
Point ProtoBot::getBombStonePosition() const
{
   return _bomb_stone_position;
}

/*!
   \param weighted_point bomb stone position
*/
void ProtoBot::setBombStonePosition(const Point& weighted_point)
{
   _bomb_stone_position = weighted_point;
}

void ProtoBot::resetBombStonePosition()
{
   // make a backup for debugging purposes
   setBombStonePositionPrevious(getBombStonePosition());

   // reset position
   _bomb_stone_position.setX(-1);
   _bomb_stone_position.setY(-1);
}

/*!
   \return previous bomb stone position
*/
Point ProtoBot::getBombStonePositionPrevious() const
{
   return _bomb_stone_position_previous;
}

/*!
   \param previous previous bomb stone position
*/
void ProtoBot::setBombStonePositionPrevious(const Point& previous)
{
   _bomb_stone_position_previous = previous;
}

void ProtoBot::resetScoringFlags()
{
   _scoring_current_hazardous = false;
   _scoring_prepare_attack = false;
   _scoring_escape = false;
   _scoring_extra = false;
   _scoring_prepare_bomb_stone = false;
   _scoring_bomb = false;
   _scoring_attack_possible = false;
   _scoring_bomb_stone_possible = false;
}

void ProtoBot::scoreFields()
{
   resetScoringFlags();

   // reset bomb bombdrop position
   resetBombStonePosition();

   // these ought to be removed at a later stage
   //   int timeForUpdateReachablePositions = 0;
   //   int timeForMarkHazardousFields = 0;
   //   int timeForUpdateEscapeScore = 0;
   //   int timeForUpdateExtraScore = 0;
   //   int timeForUpdateAttackScore = 0;
   //   int timeForUpdateBombStoneScore = 0;
   //   int timeForIsAttackPossible = 0;
   //   int timeForIsAtBombStonePosition = 0;
   //
   // QElapsedTimer timer;
   // timer.start();

   // init
   // _bot_map->updateReachablePositions(getXField(), getYField());
   _bot_map->updateReachablePositionsRandomized(getXField(), getYField());

   // timeForUpdateReachablePositions = timer.elapsed();
   // timer.restart();
   //
   //   if (_scoring_current_hazardous)
   //   {
   //      if (isDebugCurrentHazardousEnabled())
   //      {
   //         qDebug("ProtoBot::scoreFields: current field must be left!");
   //      }
   //   }

   // reset scores
   resetScores();

   // didn't re-occur
   // bugTrack3();

   // mark hazardous fields
   // timer.restart();
   markReachableFields();
   markHazardousFields();
   // markHazardousDeadEnds();
   updateHazardousTemporary(100);

   // timeForMarkHazardousFields = timer.elapsed();

   // update bomb detonation chains
   updateRemainingBombTimes();

   _scoring_current_hazardous = (getScore(getXField(), getYField()) < 0);

   // if current position must be left, 'highlight' path to a save field
   if (_scoring_current_hazardous)
   {
      // timer.restart();
      _scoring_escape = updateEscapeScore();
      // timeForUpdateEscapeScore = timer.elapsed();

      // we can't escape, which sucks
      if (!_scoring_escape)
      {
         updateLeastHazardousField();
      }
   }

   // increase individual field score by marking extras
   // calculate path to extras, find nearest extras (if max length is not exceeded)
   // then multiply path to extras by 2
   if (!_scoring_current_hazardous)
   {
      // timer.restart();
      _scoring_extra = updateExtraScore();
      // timeForUpdateExtraScore = timer.elapsed();
   }

   if (!_scoring_current_hazardous && !_scoring_extra)
   {
      // timer.restart();
      _scoring_prepare_attack = updateAttackScore();
      // timeForUpdateAttackScore = timer.elapsed();
   }

   // walk to good bomb-drop position. find that reachable field that has
   // the maximum stones to bomb away. the chosen position must have an escape
   // path.
   if (!_scoring_escape && !_scoring_extra && !_scoring_prepare_attack)
   {
      // timer.restart();
      _scoring_prepare_bomb_stone = updateBombStoneScore();
      // timeForUpdateBombStoneScore = timer.elapsed();
   }

   // timer.restart();
   _scoring_attack_possible = isAttackPossible();
   // timeForIsAttackPossible = timer.elapsed();

   // timer.restart();
   _scoring_bomb_stone_possible = isBombStonePossible();
   // timeForIsAtBombStonePosition = timer.elapsed();

   // bomb action. drop a bomb if there is a path to escape after dropping it
   if ((_scoring_prepare_bomb_stone && _scoring_bomb_stone_possible) || (_scoring_prepare_attack && _scoring_attack_possible))
   {
      if (static_cast<size_t>(_player_info->getBombCount()) > _bot_map->getBombs(_id).size())
      {
         auto option = std::make_unique<BotOption>();

         // bombs have 'leading' :)
         option->setScore(2);

         option->setAction(std::make_unique<BotBombAction>());
         _options.push_back(std::move(option));
         _scoring_bomb = true;
      }
   }

   // long distance walk action
   // presumes all other actions failed
   if (!_scoring_escape && !_scoring_extra && !_scoring_prepare_bomb_stone && !_scoring_bomb && !_scoring_prepare_attack)
   {
      int long_distance_x = _x_field;
      int long_distance_y = _y_field;

      if (evaluateLongDistance(long_distance_x, long_distance_y))
      {
         // checking the score of this field is a bit redundant since
         // we already checked for the presence of any items within the
         // given manhattan distance.. i.e. the score would be smaller
         // than 0 if there was a bomb item nearby; which is not the case.
         // anyway.. entering a hazardous field is a bad idea, so we just
         // do the check here.
         if (getScore(long_distance_x, long_distance_y) >= 0)
         {
            setScore(long_distance_x, long_distance_y, 2);
         }
      }
   }

   //   if (isDebugPossibleActionsEnabled())
   //   {
   //      // increase field scores by marking good paths (origin = current position)
   //      // remember not to mark the player's current field
   //      if (
   //            _scoring_escape
   //         || _scoring_extra
   //         || _scoring_prepare_bomb_stone
   //         || _scoring_bomb
   //         || _scoring_prepare_attack
   //      )
   //      {
   //         qDebug(
   //            "ProtoBot::scoreFields: action: escape: %d, extra: %d, "
   //            "gotobombplacepos %d, placebomb: %d, attack: %d",
   //            _scoring_escape,
   //            _scoring_extra,
   //            _scoring_prepare_bomb_stone,
   //            _scoring_bomb,
   //            _scoring_prepare_attack
   //         );
   //      }
   //   }
   //
   //   if (isDebugMapItemsEnabled())
   //      dynamic_cast<AStarMap*>(_bot_map)->debugMapItems();
   //
   //   if (isDebugScoresEnabled())
   //      debugScores();

   /*
   qDebug(
      "consumed: REACHP: %d, HAZARDF: %d, ESC: %d, EXTRA: %d, ATTSCR: %d, "
      "BSTONE: %d, ATTPSS: %d, BSTONEPSS: %d",
      timeForUpdateReachablePositions,
      timeForMarkHazardousFields,
      timeForUpdateEscapeScore,
      timeForUpdateExtraScore,
      timeForUpdateAttackScore,
      timeForUpdateBombStoneScore,
      timeForIsAttackPossible,
      timeForIsAtBombStonePosition
   );
   */
}

void ProtoBot::debugScores()
{
   qDebug("ProtoBot::debugScores(): start");

   for (int yi = 0; yi < _bot_map->getHeight(); yi++)
   {
      std::string line;
      for (int xi = 0; xi < _bot_map->getWidth(); xi++)
      {
         line += std::format("{:>3}", getScore(xi, yi));

         // put >< around current field
         if (yi == getYField() && xi + 1 == getXField())
         {
            line.append(">");
         }
         else if (yi == getYField() && xi == getXField())
         {
            line.append("<");
         }
         else
         {
            line.append("|");
         }
      }

      qDebug("%s", line.c_str());
   }

   qDebug("ProtoBot::debugScores(): end");
}

/*!
   \return \c true if escape is a good idea
*/
bool ProtoBot::updateEscapeScore()
{
   int score = 0;
   int enemy_score = 0;
   bool escape = false;
   int shortest_path_length = std::numeric_limits<int>::max();
   int path_length = 0;

   // a little hacky:
   // those safe positions that end at a position an enemy
   // is already on should be avoided
   // this is done by just increasing the calculated path length
   std::vector<Point> enemy_positions = getLivingEnemyPositions();
   std::vector<Point> future_enemy_positions = getLivingEnemyFuturePositions();

   const int field_size = _bot_map->getWidth() * _bot_map->getHeight();
   std::vector<int> enemies(static_cast<size_t>(field_size), 0);

   for (const Point& e : enemy_positions)
   {
      // having an enemy on our escape path is something to avoid
      enemies[e.x() + _bot_map->getWidth() * e.y()] += 3;

      /*
         avoid enemy positions

         +---+---+---+
         |   | E |   |
         +---+---+---+
         |XXX| ^ |XXX|
         +---+-|-+---+
         |  <-(P)->  |
         +---+---+---+
      */

      // score fields directly around an enemy with -2
      if (e.x() - 1 >= 0)
      {
         enemies[(e.x() - 1) + _bot_map->getWidth() * e.y()] += 2;
      }

      if (e.x() + 1 < _bot_map->getWidth())
      {
         enemies[(e.x() + 1) + _bot_map->getWidth() * e.y()] += 2;
      }

      if (e.y() - 1 >= 0)
      {
         enemies[e.x() + _bot_map->getHeight() * (e.y() - 1)] += 2;
      }

      if (e.y() + 1 < _bot_map->getHeight())
      {
         enemies[e.x() + _bot_map->getWidth() * (e.y() + 1)] += 2;
      }

      // score fields nearby an enemy with -1
      if (e.x() - 2 >= 0)
      {
         enemies[(e.x() - 2) + _bot_map->getWidth() * e.y()] += 1;
      }

      if (e.x() + 2 < _bot_map->getWidth())
      {
         enemies[(e.x() + 2) + _bot_map->getWidth() * e.y()] += 1;
      }

      if (e.y() - 2 >= 0)
      {
         enemies[e.x() + _bot_map->getHeight() * (e.y() - 2)] += 1;
      }

      if (e.y() + 2 < _bot_map->getHeight())
      {
         enemies[e.x() + _bot_map->getWidth() * (e.y() + 2)] += 1;
      }
   }

   // the future enemy position is damn important. if we don't set this
   // to *at least* the value the enemy positions have, both players will
   // follow each other until the time is up. that looks totally annoying
   // and surely does not help winning the game.
   for (const Point& e : future_enemy_positions)
   {
      enemies[e.x() + _bot_map->getWidth() * e.y()] += 2;
   }

   // check if neighbour positions are safe before we're going to find an
   // escape path
   std::vector<Point> neighbours = _bot_map->getReachableNeighborPositionsRandomized(getXField(), getYField());

   std::vector<Weighted<Point, int>> weighted_points;

   for (const Point& p : neighbours)
   {
      if (getScore(p.x(), p.y()) >= 0)
      {
         // set escape action
         escape = true;

         int enemy_count = enemies[p.x() + p.y() * _bot_map->getWidth()];
         weighted_points.push_back(Weighted<Point, int>(p, enemy_count));
      }
   }

   if (escape)
   {
      std::sort(weighted_points.begin(), weighted_points.end());
      Point best = weighted_points.back().getObject();

      // make a copy of the computed path
      _best_escape_path.clear();
      _best_escape_path.push_back(best);
   }

   // there's no direct neighbour position that's safe; so we have to find
   // a full escape path
   if (!escape)
   {
      // maybe use these 3 groups
      // - manhattan length 1..5
      // - manhattan length 6..10
      // - manhattan length > 11 <- choosing this group is pointless
      //                            the bot will never survive the way there
      //
      // => if escape is true, then break
      std::vector<Point> reachable_points = _bot_map->getReachablePositions();
      std::vector<Point> reachable_points_near;
      std::vector<Point> reachable_points_medium;

      // only work with reachable points within short and medium
      // distance, omit all the others
      // also pre-sort the reachable points by their manhattan distance
      // to the current field
      reachable_points_near = Map::getManhattanFiltered(Point(_x_field, _y_field), reachable_points, MANHATTAN_LENGTH_ESCAPE_NEAR_MAX);

      reachable_points_medium = Map::getManhattanFiltered(
         Point(_x_field, _y_field), reachable_points, MANHATTAN_LENGTH_ESCAPE_MEDIUM_MIN, MANHATTAN_LENGTH_ESCAPE_MEDIUM_MAX
      );

      std::vector<std::vector<Point>> reachable_point_lists;
      reachable_point_lists.push_back(reachable_points_near);
      reachable_point_lists.push_back(reachable_points_medium);

      for (const std::vector<Point>& list : reachable_point_lists)
      {
         // if one group of reachable points delivered a suitable escape path
         // then abort here
         if (!escape)
         {
            // find shortest path to non-hazardous position within the reachable positions
            for (const Point& p : list)
            {
               // reachable fields get a score of 1, checking against 0 doesn't
               // matter though
               score = getScore(p.x(), p.y());

               if (score >= 0 && (p.x() != getXField() || p.y() != getYField()))
               {
                  // since we can escape
                  // -> set escape action (phew!)
                  escape = true;

                  // now find a nice short path to that safe position
                  findPath(p.x(), p.y());
                  path_length = _path_finding.getPathLength();

                  // path was found
                  if (path_length > 0)
                  {
                     // go through every field of that path and apply the enemy
                     // effect on the path length (the more enemies on the path
                     // the worse the path length).
                     for (AStarNode* node : _path_finding.getPath())
                     {
                        enemy_score = enemies[node->getX() + node->getY() * _bot_map->getWidth()];

                        path_length *= (enemy_score + 1);
                     }

                     // if (isPathHazardous(_path_finding.getPath()))
                     path_length *= (getHazardousFieldCount(_path_finding.getPath()) + 1);

                     // path is better
                     if (path_length < shortest_path_length)
                     {
                        shortest_path_length = path_length;

                        // make a copy of the computed path
                        _best_escape_path.clear();

                        for (AStarNode* node : _path_finding.getPath())
                        {
                           _best_escape_path.insert(_best_escape_path.begin(), Point(node->getX(), node->getY()));
                        }

                        // if only one field needs to be traversed this is most likely
                        // the best path we'll get
                        if (path_length == 1)
                        {
                           break;
                        }
                     }
                  }

                  // this happens only when a previously free field is suddenly blocked
                  // by a kicked bomb => the else case is pretty boring and harmless
                  //
                  // else
                  // {
                  //    qWarning("ProtoBot::isEscapePossible(): no path found to a reachable position");
                  // }

                  clearPath();
               }
            }
         }
      }
   }

   if (escape)
   {
      for (const Point& p : _best_escape_path)
      {
         // as only the neighbored fields are examined later, increase
         // the score with each field is not required here. it's only
         // required the *current* field has a negative score and the
         // surrounding fields get more than that.
         // also do not alter the current field's score as it must be kept
         // at -1 to indicate it must be left!
         if (p.x() != getXField() || p.y() != getYField())
         {
            setScore(p.x(), p.y(), 2);
         }
      }
   }
   // else
   // {
   //     qDebug("ProtoBot::scoreFields(): no path found!");
   // }

   //   if (_debug_escape_paths)
   //   {
   //      qDebug(
   //         "ProtoBot::scoreFields(): escape from (%d, %d) via: %s",
   //         getXField(),
   //         getYField(),
   //         qPrintable(escapePathString.join(" -> "))
   //      );
   //   }

   return escape;
}

/*!
   \return \c true if going for extras is a good idea
*/
bool ProtoBot::updateExtraScore()
{
   _bot_map->updateReachableExtras();
   std::vector<Point> extra_points;
   int path_length = 0;
   bool good_idea = false;

   // let's check our memory first: if we have an extra in our memory
   // then we should focus on that and omit the standard extra evaluation
   // procedure.
   if (_memory->isExtraPositionValid())
   {
      MapItem* memory_extra = nullptr;

      memory_extra = _bot_map->getItem(_memory->getExtraPositionX(), _memory->getExtraPositionY());

      if (memory_extra && memory_extra->getType() == MapItem::Extra)
      {
         extra_points.push_back(Point(_memory->getExtraPositionX(), _memory->getExtraPositionY()));
      }
      else
      {
         // the extra is not there any more
         _memory->invalidateExtraPosition();
      }
   }

   // when we look around for extras, then don't look for extras
   // which are located too far away
   if (extra_points.empty())
   {
      extra_points = _bot_map->getReachableExtras();

      extra_points = Map::getManhattanFiltered(Point(getXField(), getYField()), extra_points, MANHATTAN_LENGTH_MAX_EXTRAS);
   }

   std::vector<Point> shortest_extra_path;

   for (const Point& p : extra_points)
   {
      // check if the extra is "occupied" by an enemy
      if (!evaluateDeadEndSituation(p.x(), p.y()))
      {
         continue;
      }

      findPath(p.x(), p.y());
      path_length = _path_finding.getPathLength();

      if (path_length > 0)
      {
         std::vector<Point> points;

         std::vector<AStarNode*> path = _path_finding.getPath();

         // there is a safe way that leads to an extra
         if (!isPathHazardous(path))
         {
            good_idea = true;

            for (AStarNode* node : path)
            {
               points.push_back(Point(node->getX(), node->getY()));
            }

            // if the extra path is empty, assign the new path immediately;
            // if the path is not empty, assign it if the path is shorter than
            // the existing one.
            if (shortest_extra_path.empty())
            {
               shortest_extra_path = points;
            }
            else
            {
               if (points.size() < shortest_extra_path.size())
               {
                  shortest_extra_path = points;
               }
            }
         }
      }

      clearPath();
   }

   if (good_idea)
   {
      // the extra point is the first position in the path
      // this one is stored in memory and re-used in the next cycle
      _memory->setExtraPosition(shortest_extra_path.front().x(), shortest_extra_path.front().y());
   }
   else
   {
      _memory->invalidateExtraPosition();
   }

   for (const Point& p : shortest_extra_path)
   {
      // only multiply the score here; hazardous fields need to be
      // kept intact
      multiplyScore(p.x(), p.y(), _bot_character->getScoreForExtras());
   }

   return good_idea;
}

/*!
   \param path path to examine
   \return \c true if path looks dangerous
*/
bool ProtoBot::isPathHazardous(const std::vector<AStarNode*>& path) const
{
   return std::ranges::any_of(path, [this](const AStarNode* node) { return getScore(node->getX(), node->getY()) < 0; });
}

/*!
   \param path path to examine
   \return \c true if path looks dangerous
*/
int ProtoBot::getHazardousFieldCount(const std::vector<AStarNode*>& path) const
{
   return static_cast<int>(std::ranges::count_if(path, [this](const AStarNode* node) { return getScore(node->getX(), node->getY()) < 0; }));
}

/*!
   \return list of living enemies
*/
std::vector<Point> ProtoBot::getLivingEnemyPositions() const
{
   std::vector<BotPlayerInfo*> enemies = getEnemies();

   std::vector<Point> enemy_positions;

   for (BotPlayerInfo* enemy : enemies)
   {
      if (!enemy->isKilled())
      {
         enemy_positions.push_back(toField(enemy->getX(), enemy->getY()));
      }
   }

   return enemy_positions;
}

/*!
   \return list of living enemies
*/
std::vector<Point> ProtoBot::getLivingEnemyFuturePositions() const
{
   std::vector<BotPlayerInfo*> enemies = getEnemies();
   std::vector<Point> enemy_positions;

   int x = 0;
   int y = 0;

   Point current;
   int8_t directions = 0;

   for (BotPlayerInfo* enemy : enemies)
   {
      if (!enemy->isKilled())
      {
         // init
         x = 0;
         y = 0;

         // read enemy directions
         current = toField(enemy->getX(), enemy->getY());
         directions = enemy->getDirections();

         // apply enemy directions to current field
         if (enemy->getDeltaX() != 0.0f)
         {
            if (directions & Constants::KeyLeft)
            {
               x = -1;
            }
            else if (directions & Constants::KeyRight)
            {
               x = 1;
            }
         }

         if (enemy->getDeltaY() != 0.0f)
         {
            if (directions & Constants::KeyUp)
            {
               y = -1;
            }
            else if (directions & Constants::KeyDown)
            {
               y = 1;
            }
         }

         x += current.x();
         y += current.y();

         if (x >= 0 && x < _bot_map->getWidth() && y >= 0 && y < _bot_map->getHeight())
         {
            Point future(x, y);
            enemy_positions.push_back(future);
         }
      }
   }

   return enemy_positions;
}

/*!
   \return list of enemies
*/
std::vector<BotPlayerInfo*> ProtoBot::getEnemies() const
{
   std::vector<BotPlayerInfo*> enemies;

   for (const auto& [id, player] : *_player_info_map)
   {
      if (player->getId() != _id)
      {
         enemies.push_back(player.get());
      }
   }

   return enemies;
}

/*!
   \return \c true if going to a bomb drop position looks suitable
*/
bool ProtoBot::updateBombStoneScore()
{
   if (isNoBombInfectionActive())
   {
      return false;
   }

   // init
   int path_finding_loops1 = 0;
   // int timePathFinding = 0;
   // QElapsedTimer measureTime;
   // measureTime.start();
   const std::vector<int> stones_to_be_bombed = _bot_map->getStonesToBeBombedMap();
   bool found_safe_drop_position = false;
   std::vector<Point> reachable_points = _bot_map->getReachablePositions();
   Point previous_bomb_stone_position = getBombStonePositionPrevious();

   // do the same optimization as for the extras.. check the bot's memory
   // for an existing bomb stone position. if that position is reachable
   // and has the same "bomb stone count" as it had when we stored it in
   // the bot's memory, there is absolutely no need to compute a new position.
   bool use_memory_position = false;
   if (_memory->isBombStonePositionValid())
   {
      int score = _bot_map->getStoneCountAroundPoint(
         _memory->getBombStonePositionX(), _memory->getBombStonePositionY(), _player_info->getFlameCount()
      );

      for (const Point& p : reachable_points)
      {
         if (p.x() == _memory->getBombStonePositionX() && p.y() == _memory->getBombStonePositionY() &&
             score == _memory->getBombStoneCount())
         {
            // the bomb stone position stored in memory has still is still
            // reachable and has the same score as the bot remembered. there
            // does not seem to be any reason we shouldn't use it for all
            // further computations...
            use_memory_position = true;
         }
      }
   }

   // collect points to be weighted
   std::vector<Weighted<Point, int>> weighted_points;

   if (use_memory_position)
   {
      Point p(_memory->getBombStonePositionX(), _memory->getBombStonePositionY());

      Weighted<Point, int> weighted(p, _memory->getBombStoneCount());
      weighted_points.push_back(weighted);
   }
   else
   {
      // this is the default way to compute bomb stone positions
      // i.e. they way positions are computed without using the bot's
      // memory
      for (const Point& p : reachable_points)
      {
         int score = _bot_map->getStoneCountAroundPoint(p.x(), p.y(), _player_info->getFlameCount());

         // "stones to be bombed" contains a value = -1 if the stone is going
         // to be bombed away by another bomb
         score += stones_to_be_bombed[p.y() * _bot_map->getWidth() + p.x()];

         if (score > 0)
         {
            if (!evaluateDeadEndSituation(p.x(), p.y()))
            {
               continue;
            }

            /*

            // if score counts (score > 0), multiply it with number of extras in the stones
            score *= _bot_map->getExtraStoneCountAroundPoint(
                  p.x(),
                  p.y(),
                  _player_info->getFlameCount(),
                  getExtraShakeIds()
               );

            */

            // this is done in order to avoid "losing" the bomb stone position.
            // the reason for this is the fact positions may have the same score
            // but they're sorted differently depending on the bot's position. the
            // effect of this is, the bot runs to the one position, detects an equal
            // score at another position, runs into the other direction, detects an
            // equal score again.. and repeats that procedure.. all the time..
            // this can be avoided by giving the previous position a higher score.
            // this is only done when the previous position is still included in the
            // list of available positions.
            if (p == previous_bomb_stone_position)
            {
               if (score > 0)
               {
                  score++;
               }
            }

            // store weighted point
            Weighted<Point, int> weighted(p, score);
            weighted_points.push_back(weighted);
         }
      }
   }

   // now sort the weighted positions
   std::sort(weighted_points.begin(), weighted_points.end());

   // analyze current pos
   Point current = Point(getXField(), getYField());

   int current_score = _bot_map->getStoneCountAroundPoint(current.x(), current.y(), _player_info->getFlameCount());

   Weighted<Point, int> weighted_current(current, current_score);

   if (!weighted_points.empty())
   {
      if (current_score >= weighted_points.at(0).getWeight())
      {
         weighted_points.insert(weighted_points.begin(), weighted_current);
      }
   }

   // find a place to hide from the bomb either by
   // going one field up or down or by going a field left or right
   //
   // start with the best point and go to the last
   int flame_size = _player_info->getFlameCount();

   Point weighted_point;
   for (const Weighted<Point, int>& weighted : weighted_points)
   {
      weighted_point = weighted.getObject();

      // optimization:
      // check every bloody point in case we have less than 10 of them
      // or check all points with a manhattan length below 5
      int manhattan_distance = Map::getManhattanLength(getXField(), getYField(), weighted_point.x(), weighted_point.y());

      if (weighted_points.size() < 10  // works for memorized position, too
          || manhattan_distance <= 5 || weighted_point == previous_bomb_stone_position)
      {
         // take ALL reachable positions and remove those that will be burned
         // by the bomb as soon it will have been placed. if there's just
         // ONE position left that we're able to find a path to, we're cool.
         std::vector<Point> reachable_positions_filtered =
            reachablePositionsLeft(weighted_point.x(), weighted_point.y(), flame_size, reachable_points);

         // sort this list of reachable points by starting at the detonation
         // point;
         // after doing that pick a limited number of points suited for
         // further inspection.
         std::vector<Weighted<Point, int>> reachable_positions_by_manhattan_length;
         for (const Point& p : reachable_positions_filtered)
         {
            int manhattan = -(p - weighted_point).manhattanLength();
            reachable_positions_by_manhattan_length.push_back(Weighted<Point, int>(p, manhattan));
         }
         std::sort(reachable_positions_by_manhattan_length.begin(), reachable_positions_by_manhattan_length.end());
         reachable_positions_filtered.clear();

         int iterations = 0;
         int max = 30;
         for (const Weighted<Point, int>& w : reachable_positions_by_manhattan_length)
         {
            reachable_positions_filtered.push_back(w.getObject());

            if (iterations > max)
            {
               break;
            }

            iterations++;
         }

         // then find the way to the possibly shortest safe point.
         // this 1st path-finding has just one purpose which is to
         // ensure there is a way to escape from the bomb-drop-position
         // in spe to the safe position (safe_point)
         for (const Point& safe_point : reachable_positions_filtered)
         {
            // score path to bomb drop position
            findPathFromTo(weighted_point.x(), weighted_point.y(), safe_point.x(), safe_point.y());

            int path_length = _path_finding.getPathLength();
            path_finding_loops1++;

            if (path_finding_loops1 > 300)
            {
               qWarning("ProtoBot::updateBombStoneScore(): cpu usage exceeded");
            }

            // a safe position must not be our current position
            if (path_length > 0)
            {
               bool safe_path = !isPathHazardous(_path_finding.getPath());
               clearPath();

               if (safe_path)
               {
                  // optimization:
                  // if we're already on the field we want to bomb
                  // there's no need to to find a path
                  if (weighted_point.x() == getXField() && weighted_point.y() == getYField())
                  {
                     // assign bomb drop position
                     setBombStonePosition(weighted_point);

                     found_safe_drop_position = true;
                  }
                  else
                  {
                     findPath(weighted_point.x(), weighted_point.y());

                     std::vector<AStarNode*> found_path = _path_finding.getPath();

                     if (!found_path.empty())
                     {
                        for (AStarNode* node : found_path)
                        {
                           multiplyScore(node->getX(), node->getY(), _bot_character->getScoreForPrepareBombDrop());
                        }

                        // assign bomb drop position
                        setBombStonePosition(weighted_point);

                        // yup. we can drop a bomb
                        found_safe_drop_position = true;
                     }
                     else
                     {
                        qDebug(
                           "ProtoBot::updateBombStoneScore(): path finding failed "
                           "finding a path from (%d;%d) to (%d;%d)",
                           _x_field,
                           _y_field,
                           weighted_point.x(),
                           weighted_point.y()
                        );
                     }

                     clearPath();
                  }
               }
            }
            else
            {
               clearPath();
            }

            if (found_safe_drop_position)
            {
               break;
            }
         }

         if (found_safe_drop_position)
         {
            break;
         }
      }
   }

   //   timePathFinding = measureTime.elapsed();
   //
   //   if (timePathFinding > 1000)
   //   {
   //      qWarning(
   //         "bool ProtoBot::updateBombStoneScore(): cpu usage critical, %dms "
   //         "loops1: %d, loops2: %d",
   //         timePathFinding,
   //         path_finding_loops1,
   //         path_finding_loops2
   //      );
   //   }

   if (found_safe_drop_position)
   {
      int stone_count =
         _bot_map->getStoneCountAroundPoint(getBombStonePosition().x(), getBombStonePosition().y(), _player_info->getFlameCount());

      _memory->setBombStonePosition(getBombStonePosition().x(), getBombStonePosition().y());

      _memory->setBombStoneCount(stone_count);
   }
   else
   {
      _memory->invalidateBombStonePosition();
   }

   return found_safe_drop_position;
}

/*!
   \return \c true if attack preparation sounds good
*/
bool ProtoBot::updateAttackScore()
{
   if (isNoBombInfectionActive())
   {
      return false;
   }

   bool attack = false;

   if (_bot_map->isBombAmountConsumed(_id, _player_info->getBombCount()))
   {
      // locate enemies
      _enemy_positions = getLivingEnemyPositions();
      std::vector<Point> reachable_points = _bot_map->getReachablePositions();
      std::vector<Point> reachable_enemies;

      for (const Point& reachable : reachable_points)
      {
         if (std::ranges::find(_enemy_positions, reachable) != _enemy_positions.end())
         {
            reachable_enemies.push_back(reachable);
         }
      }

      if (!reachable_enemies.empty())
      {
         int shortest_path_length = std::numeric_limits<int>::max();
         int path_length = 0;

         // find shortest path to enemy
         for (const Point& enemy_position : reachable_enemies)
         {
            findPath(enemy_position.x(), enemy_position.y());

            path_length = _path_finding.getPathLength();

            // choose shortest path
            if (path_length < shortest_path_length)
            {
               attack = true;

               shortest_path_length = path_length;

               // make a copy of the computed path
               _best_attack_path.clear();

               // if the path length is zero, we're most likely standing on our
               // enemy right now
               if (path_length == 0)
               {
                  _best_attack_path.push_back(Point(getXField(), getYField()));
               }
               else
               {
                  for (AStarNode* node : _path_finding.getPath())
                  {
                     _best_attack_path.insert(_best_attack_path.begin(), Point(node->getX(), node->getY()));
                  }
               }

               // if only one field needs to be traversed this is most likely
               // the best path we'll get
               if (path_length <= 1)
               {
                  clearPath();
                  break;
               }
            }

            clearPath();
         }

         if (!_best_attack_path.empty())
         {
            for (const Point& p : _best_attack_path)
            {
               // as only the neighbored fields are examined later, increase
               // the score with each field is not required here. it's only
               // required the *current* field has a negative score and the
               // surrounding fields get more than that.
               // also do not alter the current field's score as it must be kept
               // at -1 to indicate it must be left!
               if (p.x() != getXField() || p.y() != getYField())
               {
                  multiplyScore(p.x(), p.y(), _bot_character->getScoreForAttack());
               }
            }
         }
      }
   }

   return attack;
}

/*!

   if the neighbor field score is not any more interesting than
   the current one...
   and we're on our way to a bomb some stones away...
   but we're not yet on that bomb stone position's center
   though already on the bomb score position...
   then trick the code to keep on walking just a little more
   until we're really in the center of a field

   \return \c true if score should be overriden
*/
bool ProtoBot::overrideFieldScore()
{
   int x_field = getXField();
   int y_field = getYField();

   bool override = false;

   if (_scoring_prepare_bomb_stone)
   {
      if (!_scoring_bomb_stone_possible)
      {
         Point bomb_stone_position = getBombStonePosition();

         if (bomb_stone_position.x() == x_field && bomb_stone_position.y() == y_field)
         {
            override = true;
         }
      }
   }

   return override;
}

void ProtoBot::chooseNextField()
{
   int x_field = getXField();
   int y_field = getYField();

   // list reachable neighbor positions (up, down, left, right)
   // proper implementation: find shortest path to a save position!
   std::vector<Point> neighbors = _bot_map->getReachableNeighborPositions(x_field, y_field);

   // if neighbors count == 0 consider yourself dead
   if (!neighbors.empty())
   {
      // init max_score with current field's score
      int max_score = getScore(x_field, y_field);
      int field_score = -1;
      bool other_field_score_is_higher = false;

      // calculate the maximum score and add all neighbors with the
      // maximum score to the bestpoints
      for (const Point& p : neighbors)
      {
         field_score = getScore(p.x(), p.y());

         if (field_score > max_score)
         {
            other_field_score_is_higher = true;
            max_score = field_score;
         }
      }

      // not sure if the "bestpoints" part is really needed
      // its purpose is to "keep on track" if there are more than one
      // points with the best score => just walk into the same direction
      std::vector<Point> best_points;
      for (const Point& p : neighbors)
      {
         field_score = getScore(p.x(), p.y());

         if (field_score == max_score)
         {
            best_points.push_back(p);
         }
      }

      if (!best_points.empty())
      {
         if (std::ranges::find(best_points, _last_target) != best_points.end())
         {
            _transiterate_target_x = _last_target.x();
            _transiterate_target_y = _last_target.y();
         }
         else
         {
            _last_target = best_points.at(randomize(0, static_cast<int>(best_points.size()) - 1));
         }
      }

      /*
      qDebug(
         "CurrentHazardous: %d, PrepareAttack: %d, Escape: %d, Extra: %d, "
         "PrepareBombStone: %d, Bomb: %d, AttackPossible: %d, AtBombStonePosition: %d",
         _scoring_current_hazardous,
         _scoring_prepare_attack,
         _scoring_escape,
         _scoring_extra,
         _scoring_prepare_bomb_stone,
         _scoring_bomb,
         _scoring_attack_possible,
         _scoring_at_bomb_stone_position
      );
      */

      if (!other_field_score_is_higher)
      {
         other_field_score_is_higher = overrideFieldScore();
      }

      if (other_field_score_is_higher)
      {
         // only walk into a field that does NOT explode within the next second ;)
         // if it's 1000ms, we can enter it anyway, if our current remaining
         // field time is even worse.. there's nothing to lose :)
         int target_remaining = getRemainingBombTime(_transiterate_target_x, _transiterate_target_y);

         int current_remaining = getRemainingBombTime(_x_field, _y_field);

         if (target_remaining > 1000 || target_remaining >= current_remaining)
         {
            /*
            qDebug(
               "target: %d, current: %d",
               target_remaining,
               current_remaining
            );
            */

            // add walk action
            auto option = std::make_unique<BotOption>();
            option->setScore(1);
            option->setAction(getWalkActionInstance());
            _options.push_back(std::move(option));
         }
      }
   }

   // wait! maybe we can kick our way out :)
   else
   {
      int x = _x_field;
      int y = _y_field;

      if (isKickEscapePossible(x, y))
      {
         _transiterate_target_x = x;
         _transiterate_target_y = y;

         // add walk action
         auto option = std::make_unique<BotOption>();
         option->setScore(1);
         option->setAction(getWalkActionInstance());
         _options.push_back(std::move(option));
      }
   }
}

void ProtoBot::think()
{
   Bot::think();

   // calc scores for all reachable fields
   scoreFields();

   chooseNextField();

   if (_options.empty())
   {
      auto option = std::make_unique<BotOption>();
      option->setAction(std::make_unique<BotIdleAction>());
      _options.push_back(std::move(option));
   }
}

/*!
   \return new walk action
*/
std::unique_ptr<BotWalkAction> ProtoBot::getWalkActionInstance()
{
   int8_t keys_pressed = computeWalkKeys();

   auto action = std::make_unique<BotWalkAction>();
   action->setWalkKeys(keys_pressed);

   //   if (isDebugWalkActionEnabled())
   //   {
   //      qDebug(
   //         "ProtoBot::getWalkActionInstance(): x: %d; y: %d; (%f, %f); "
   //         "tx:%d, ty: %d;",
   //         getXField(),
   //         getYField(),
   //         _x,
   //         _y,
   //         _transiterate_target_x,
   //         _transiterate_target_y
   //      );
   //   }
   //
   //   if (isDebugKeysPressedEnabled())
   //   {
   //      QString keysPressedString =
   //         QString("keysPressed: %1|%2|%3|%4")
   //            .arg( (keys_pressed & Constants::KeyUp   ) ? "up"    : "--")
   //            .arg( (keys_pressed & Constants::KeyDown ) ? "down"  : "----")
   //            .arg( (keys_pressed & Constants::KeyLeft ) ? "left"  : "----")
   //            .arg( (keys_pressed & Constants::KeyRight) ? "right" : "----");
   //
   //      qDebug("%s", qPrintable(keysPressedString));
   //   }

   // store latest walk information
   setBotKeysPressed(keys_pressed);

   return action;
}

void ProtoBot::clearPath()
{
   dynamic_cast<AStarMap*>(_bot_map)->clearNodes();
}

void ProtoBot::debugPathWithMap()
{
   dynamic_cast<AStarMap*>(_bot_map)->debugPath(_path_finding.getPath());
}

/*!
   \param bot_map bot map
*/
void ProtoBot::setBotMap(BotMap* bot_map)
{
   Bot::setBotMap(bot_map);

   // reinit fields
   const auto size = static_cast<size_t>(bot_map->getWidth() * bot_map->getHeight());
   _field_scores.assign(size, 0);
   _field_bomb_times.assign(size, 0);
   _hazardous_temporary.assign(size, 0);

   _path_finding.setMap(dynamic_cast<AStarMap*>(_bot_map));
}

/*!
   \param target x
   \param target y
*/
void ProtoBot::findPath(int target_x, int target_y)
{
   findPathFromTo(getXField(), getYField(), target_x, target_y);
}

/*!
   \param start_x start x position
   \param start_y start y position
   \param target_x target x position
   \param target_y target y position
*/
void ProtoBot::findPathFromTo(int start_x, int start_y, int target_x, int target_y)
{
   // init a star map
   dynamic_cast<AStarMap*>(_bot_map)->buildNodes();

   // init pathfinding
   _path_finding.setStart(start_x, start_y);
   _path_finding.setTarget(target_x, target_y);

   // find path
   _path_finding.findPath();

   // debug map plus map with path
   if (_debug_paths)
   {
      _path_finding.debugPathShort();

      // _path_finding.debugPath();
      // debugPathWithMap();
   }
}

/*!
  \param width map width
  \param height map height
  \return map
*/
std::unique_ptr<BotMap> ProtoBot::createMap(int width, int height)
{
   return std::make_unique<AStarMap>(width, height);
}

/*!
   \param map player info map
*/
void ProtoBot::setPlayerInfoMap(PlayerInfoMap* map)
{
   _player_info_map = map;
}

/*!
   \return insult message handler
*/
ProtoBotInsults* ProtoBot::getInsults()
{
   return _insults.get();
}

/*!
   \return insult message handler
*/
const std::vector<int>& ProtoBot::getExtraShakeIds() const
{
   return _extra_shake_ids;
}

/*!
   \param x x pos
   \param y y pos
   \param flames no of flames
   \param reachable_positions reachable positions
   \return \c true if a bomb drop is safe
*/
std::vector<Point> ProtoBot::reachablePositionsLeft(int x, int y, int flames, const std::vector<Point>& reachable_positions) const
{
   int direction_x = 0;
   int direction_y = 0;

   std::vector<Point> copy = reachable_positions;

   // filter those out that are hazardous (reachable positions are unique)
   std::erase_if(copy, [this](const Point& p) { return getScore(p.x(), p.y()) < 0; });

   for (Constants::Direction direction : _bot_map->getDirectionsAndCurrent())
   {
      switch (direction)
      {
         case Constants::DirectionUp:
            direction_x = 0;
            direction_y = -1;
            break;
         case Constants::DirectionDown:
            direction_x = 0;
            direction_y = 1;
            break;
         case Constants::DirectionLeft:
            direction_x = -1;
            direction_y = 0;
            break;
         case Constants::DirectionRight:
            direction_x = 1;
            direction_y = 0;
            break;
         default:
            direction_x = 0;
            direction_y = 0;
            break;
      }

      // example:
      //    2 flames
      //    => check x+0, y
      //    => check x+1, y
      //    => check x+2, y
      for (int i = 1; i <= flames; i++)
      {
         int position_x = x + i * direction_x;
         int position_y = y + i * direction_y;

         auto point_iterator = std::ranges::find(copy, Point(position_x, position_y));

         if (point_iterator != copy.end())
         {
            copy.erase(point_iterator);
         }
      }
   }

   return copy;
}

/*!
   \return \c true if kick is a good idea
*/
bool ProtoBot::isKickPossible()
{
   bool kick = false;

   if (_player_info->isKickEnabled())
   {
      // int enemiesHitCount = 0;

      // ok, we can kick - now what?

      // there are mostly 2 criteria:
      //
      // 1) the bomb must be within a certain distance, let's say 4-5 fields
      // 2) we must be able to reach that bomb before it explodes
      // 3) the bomb must be reachable
      // 4) there must be a bomb which will hit an enemy
      // 5) if it hits and explodes at the enemy position, it should not kill us

      // FOR EACH BOMB

      // checking the criteria
      //
      // 1) calculating the manhattan length to each bomb should be a good filter
      //    to avoid looking at those bombs totally out of our reach

      // 2) when will it explode? will we make it there in time?

      // 3) compute a path to that bomb

      // 4) will it collide with an enemy or will it hit an enemy at its
      //    destination position? the more enemies are hit the better
      //    => enemiesHitCount++

      // 5) will it do no harm to us

      // the score should be the distance to the bomb multiplied with the
      // number of enemies hit

      // => kick the best option, set "kick" flag to true
   }

   return kick;
}

/*!
   \return \c true if infection is active
*/
bool ProtoBot::isNoBombInfectionActive() const
{
   bool active = false;

   if (_player_info->getDisease())
   {
      if (_player_info->getDisease()->getType() == Constants::SkullNoBomb)
      {
         active = true;
      }
   }

   return active;
}

/*!
   \return \c true if infection is active
*/
bool ProtoBot::isInfectionActive(Constants::SkullType skull_type) const
{
   bool active = false;

   if (_player_info->getDisease())
   {
      if (_player_info->getDisease()->getType() == skull_type)
      {
         active = true;
      }
   }

   return active;
}

/*!
   \return \c true debug paths is active
*/
bool ProtoBot::isDebugPathsEnabled() const
{
   return _debug_paths;
}

/*!
   \param debug if debug paths is active
*/
void ProtoBot::setDebugPathsEnabled(bool debug)
{
   _debug_paths = debug;
}

/*!
   \return \c true debug for escape paths is active
*/
bool ProtoBot::isDebugEscapePathsEnabled() const
{
   return _debug_escape_paths;
}

/*!
   \param enabled debug for escape paths enabled
*/
void ProtoBot::setDebugEscapePathsEnabled(bool enabled)
{
   _debug_escape_paths = enabled;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugMapItemsEnabled(bool enabled)
{
   _debug_map_items = enabled;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugMapItemsEnabled() const
{
   return _debug_map_items;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugScoresEnabled() const
{
   return _debug_scores;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugScoresEnabled(bool value)
{
   _debug_scores = value;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugKeysPressedEnabled() const
{
   return _debug_keys_pressed;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugKeysPressedEnabled(bool value)
{
   _debug_keys_pressed = value;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugPossibleActionsEnabled() const
{
   return _debug_possible_actions;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugPossibleActionsEnabled(bool value)
{
   _debug_possible_actions = value;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugExecutedActionsEnabled() const
{
   return _debug_executed_actions;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugExecutedActionsEnabled(bool value)
{
   _debug_executed_actions = value;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugCurrentHazardousEnabled() const
{
   return _debug_current_hazardous;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugCurrentHazardousEnabled(bool value)
{
   _debug_current_hazardous = value;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugWalkActionEnabled() const
{
   return _debug_walk_action;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugWalkActionEnabled(bool value)
{
   _debug_walk_action = value;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugBombDropEnabled() const
{
   return _debug_bomb_drop;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugBombDropEnabled(bool value)
{
   _debug_bomb_drop = value;
}

/*!
   \param enabled debug flag is enabled
*/
void ProtoBot::setDebugBreakpointEnabled(bool enabled)
{
   _debug_breakpoint = enabled;
}

/*!
   \return \c true debug flag is active
*/
bool ProtoBot::isDebugBreakpointEnabled() const
{
   return _debug_breakpoint;
}

/*!
   \param x x to go
   \param y y to go
   \return \c true if we can get outta here
*/
bool ProtoBot::isKickEscapePossible(int& x, int& y)
{
   bool escape_possible = false;

   if (_player_info->isKickEnabled())
   {
      // check for bombs on up, down, left and right
      if ((y - 2 >= 0) && _bot_map->getItem(x, y - 1) && _bot_map->getItem(x, y - 1)->getType() == MapItem::Bomb &&
          _bot_map->getItem(x, y - 2) == nullptr)
      {
         escape_possible = true;
         y--;
      }

      else if ((y + 2 < _bot_map->getHeight()) && _bot_map->getItem(x, y + 1) && _bot_map->getItem(x, y + 1)->getType() == MapItem::Bomb &&
               _bot_map->getItem(x, y + 2) == nullptr)
      {
         escape_possible = true;
         y++;
      }

      else if ((x - 2 >= 0) && _bot_map->getItem(x - 1, y) && _bot_map->getItem(x - 1, y)->getType() == MapItem::Bomb &&
               _bot_map->getItem(x - 2, y) == nullptr)
      {
         escape_possible = true;
         x--;
      }

      else if ((x + 2 < _bot_map->getWidth()) && _bot_map->getItem(x + 1, y) && _bot_map->getItem(x + 1, y)->getType() == MapItem::Bomb &&
               _bot_map->getItem(x + 2, y) == nullptr)
      {
         escape_possible = true;
         x++;
      }
   }

   return escape_possible;
}

/*!
   \param next_x_field next field x position to choose
   \param next_y_field next field y position to choose
   \return \c true if another field should be chosen
*/
bool ProtoBot::evaluateLongDistance(int& next_x_field, int& next_y_field) const
{
   /*

      we get into this function when there's nothing to do within
      our current manhattan distance range (mostly a radius of 5, 6 fields
      around the player).

      so we can assume the player stands freely, its movement is only
      limited by blocks. therefore we just need to shift left or right,
      up or down if there's any obstacle.

      [X] [X]           [X]P[X]           [X] [X]
         P                                     P
      [X] [X]           [X] [X]           [X] [X]

      all movement      shift left        shift up
      possible          and right         and down
                        blocked           blocked


                                 hint: the walls do not matter :)

                                 XXXXXXX                    X
                                                            X
                                 [X]P[X]           [X] [X]  X
                                                        P   X
                                 [X] [X]           [X] [X]  X
                                                            X
   */

   bool move_possible = false;

   // code only applies for large maps
   //
   // .. actually... why should it!?
   // if (_bot_map->getWidth() > 13)
   {
      /*

         manhattan distance from current point (0)

                     5
                   5 4 5
                 5 4 3 4 5
               5 4 3 2 3 4 5
             5 4 3 2 1 2 3 4 5
           5 4 3 2 1 0 1 2 3 4 5
             5 4 3 2 1 2 3 4 5
               5 4 3 2 3 4 5
                 5 4 3 4 5
                   5 4 5
                     5
      */

      // analyze the manhattan range used by the short distance functions
      // to locate all items that are NOT block items.. in case we find
      // one single iteam within the short distance range, there's no point
      // to continue with the long distance evaluations.
      int start_x = _x_field;
      int start_y = _y_field;

      int ml = 5;
      int diameter = (ml * 2) + 1;

      int fields = 1;
      int x_offset = 0;
      int y_offset = -ml;
      int test_x = 0;
      int test_y = 0;

      MapItem* item = nullptr;
      MapItem::ItemType type = MapItem::Unknown;

      for (int y = 0; y < diameter; y++)
      {
         test_y = start_y + y_offset;

         for (int x = 0; x < fields; x++)
         {
            test_x = start_x + x + x_offset;

            // check 1 point for the presence of anything interesting
            if (test_x >= 0 && test_x < _bot_map->getWidth() && test_y >= 0 && test_y < _bot_map->getHeight())
            {
               item = _bot_map->getItem(test_x, test_y);

               if (item)
               {
                  type = item->getType();

                  // a nice clean ragequit
                  // => there IS actually something to do within our
                  //    given manhattan distance.. so hold yer horses.
                  if (type != MapItem::Block)
                  {
                     return false;
                  }
               }
            }
         }

         // next line
         y_offset++;

         // increase width by two fields until center is reached.
         // also move to the left until the center is reached
         // then move to the right again.
         if (y_offset <= 0)
         {
            x_offset--;
            fields += 2;
         }
         else
         {
            x_offset++;
            fields -= 2;
         }
      }

      // if there's an enemy within short distance range, there's also no
      // point to evaluate any further than here. if there's an enemy in
      // short distance and we do not plan to attack him, we might have
      // good reasons to do so => abort.
      std::vector<Point> enemy_positions = getLivingEnemyPositions();
      for (const Point& p : enemy_positions)
      {
         if (BotMap::getManhattanLength(p.x(), p.y(), _x_field, _y_field) < MANHATTAN_LENGTH_MAX_ATTACK)
         {
            // another ragequit (see reason above)
            return false;
         }
      }

      // start the search for a little action on the field
      int target_x = -1;
      int target_y = -1;

      // find extras located somewhere
      if (target_x < 0)
      {
         for (int y = 0; y < _bot_map->getHeight(); y++)
         {
            for (int x = 0; x < _bot_map->getWidth(); x++)
            {
               if (_bot_map->getItem(x, y) && _bot_map->getItem(x, y)->getType() == MapItem::Extra)
               {
                  target_x = x;
                  target_y = y;
                  break;
               }
            }
         }
      }

      // find stones located somewhere
      if (target_x < 0)
      {
         for (int y = 0; y < _bot_map->getHeight(); y++)
         {
            for (int x = 0; x < _bot_map->getWidth(); x++)
            {
               if (_bot_map->getItem(x, y) && _bot_map->getItem(x, y)->getType() == MapItem::Stone)
               {
                  target_x = x;
                  target_y = y;
                  break;
               }
            }
         }
      }

      // find stones located somewhere
      if (target_x < 0)
      {
         if (!enemy_positions.empty())
         {
            target_x = enemy_positions.at(0).x();
            target_y = enemy_positions.at(0).y();
         }
      }

      if (target_x >= 0)
      {
         move_possible = true;

         bool up = false;
         bool down = false;
         bool left = false;
         bool right = false;

         up = (target_y < _y_field);
         down = (target_y > _y_field);
         left = (target_x < _x_field);
         right = (target_x > _x_field);

         // if we cannot go up or down
         // => shift to the side
         if (up && (next_y_field - 1 >= 0))
         {
            if (_bot_map->isPositionBlocked(next_x_field, next_y_field - 1))
            {
               if (right)
               {
                  next_x_field++;
               }
               else
               {
                  next_x_field--;
               }
            }
            else
            {
               next_y_field--;
            }
         }
         else if (down && (next_y_field + 1 < _bot_map->getHeight()))
         {
            if (_bot_map->isPositionBlocked(next_x_field, next_y_field + 1))
            {
               if (right)
               {
                  next_x_field++;
               }
               else
               {
                  next_x_field--;
               }
            }
            else
            {
               next_y_field++;
            }
         }

         // if we cannot go left or right
         // => shift up or down
         else if (left && (next_x_field - 1 >= 0))
         {
            if (_bot_map->isPositionBlocked(next_x_field - 1, next_y_field))
            {
               if (down)
               {
                  next_y_field++;
               }
               else
               {
                  next_y_field--;
               }
            }
            else
            {
               next_x_field--;
            }
         }
         else if (right && (next_x_field + 1 < _bot_map->getWidth()))
         {
            if (_bot_map->isPositionBlocked(next_x_field + 1, next_y_field))
            {
               if (down)
               {
                  next_y_field++;
               }
               else
               {
                  next_y_field--;
               }
            }
            else
            {
               next_x_field++;
            }
         }
      }
   }

   return move_possible;
}

/*!
   \param start_x kick start x position
   \param start_y kick start y position
   \param direction kick direction
   \param flames no of flames
*/
void ProtoBot::bombKicked(int start_x, int start_y, Constants::Direction direction, int flames)
{
   bool hit_something = false;
   int xi = start_x;
   int yi = start_y;
   int xi_previous = 0;
   int yi_previous = 0;
   int i = 1;
   MapItem* item = nullptr;

   // compute kick end position
   while (!hit_something)
   {
      xi_previous = xi;
      yi_previous = yi;

      switch (direction)
      {
         case Constants::DirectionUp:
         {
            yi = start_y - i;
            break;
         }

         case Constants::DirectionDown:
         {
            yi = start_y + i;
            break;
         }

         case Constants::DirectionLeft:
         {
            xi = start_x - i;
            break;
         }

         case Constants::DirectionRight:
         {
            xi = start_x + i;
            break;
         }

         default:
         {
            break;
         }
      }

      if (xi >= 0 && xi < _bot_map->getWidth() && yi >= 0 && yi < _bot_map->getHeight())
      {
         item = _bot_map->getItem(xi, yi);

         if (item && item->isBlocking())
         {
            hit_something = true;
         }
         else
         {
            for (const auto& [id, player] : *_player_info_map)
            {
               if (!player->isKilled())
               {
                  int px = static_cast<int>(std::floor(player->getX()));
                  int py = static_cast<int>(std::floor(player->getY()));

                  if (px == xi && py == yi)
                  {
                     hit_something = true;
                  }
               }
            }
         }
      }
      else
      {
         hit_something = true;
      }

      i++;
   }

   if (xi_previous >= 0 && xi_previous < _bot_map->getWidth() && yi_previous >= 0 && yi_previous < _bot_map->getHeight())
   {
      // qDebug("mark %d, %d with %d flames hazardous", xi_previous, yi_previous, flames);
      markHazardousTemporary(xi_previous, yi_previous, 1500, flames);
   }
}

/*!
   \return \c true if a better field has been found
*/
bool ProtoBot::updateLeastHazardousField()
{
   bool found_something = false;

   // all reachable fields must suck
   std::vector<Point> reachable = _bot_map->getReachablePositions();

   // the number of reachable fields must be pretty limited
   int reachable_count = static_cast<int>(reachable.size());
   if (reachable_count <= 4 && reachable_count >= 2)
   {
      bool proceed = true;
      for (const Point& p : reachable)
      {
         if (getScore(p.x(), p.y()) >= 0)
         {
            proceed = false;
            break;
         }
      }

      // if they all suck, choose the one that sucks least
      if (proceed)
      {
         Point best;
         int remaining = 0;
         int remaining_temp = 0;

         for (const Point& p : reachable)
         {
            remaining_temp = getRemainingBombTime(p.x(), p.y());

            if (remaining_temp > remaining)
            {
               best = p;
               remaining = remaining_temp;
            }
         }

         // find a path to the best field if it differs from our current field
         if (best.x() != _x_field || best.y() != _y_field)
         {
            if (remaining > 0)
            {
               findPath(best.x(), best.y());

               std::vector<AStarNode*> path = _path_finding.getPath();

               if (!path.empty())
               {
                  found_something = true;

                  for (AStarNode* node : path)
                  {
                     if (node->getX() != _x_field && node->getY() != _y_field)
                     {
                        // make the bot choose the better field
                        setScore(node->getX(), node->getY(), 2);
                     }
                  }
               }

               clearPath();
            }
         }
      }
   }

   return found_something;
}

/*!
   \param start analysis at x
   \param start analysis at y
   \param recursion_depth current recursion depth
   \param end dead end's end point
   \param direction recursion direction
   \return list of points belonging to a "dead end"
*/
std::vector<Point> ProtoBot::analyzeDeadEnd(int x, int y, Point& end, int recursion_depth, const Point& direction) const
{
   recursion_depth++;

   std::vector<Point> points;
   int reachable_neighbours = 4;
   Point opening_direction1;
   Point opening_direction2;
   MapItem* item = nullptr;
   int xi = 0;
   int yi = 0;

   /*

      default dead end situations

         [X][X]   [X][X]
         [X]         [X]
         [X][X]   [X][X]

         [X][X][X]
         [X]   [X]

         [X]   [X]
         [X][X][X]


      2 field dead ends

         [X][X][X]   [X][X][X]
         [X]               [X]
         [X][X][X]   [X][X][X]

         [X][X][X]
         [X]   [X]
         [X]   [X]

         [X]   [X]
         [X]   [X]
         [X][X][X]


      and so on...

   */

   // examine current position
   for (const Point& neighbor_direction : _directions)
   {
      xi = x + neighbor_direction.x();
      yi = y + neighbor_direction.y();

      if (xi >= 0 && xi < _bot_map->getWidth() && yi >= 0 && yi < _bot_map->getHeight())
      {
         item = _bot_map->getItem(xi, yi);

         if (item && item->isBlocking())
         {
            reachable_neighbours--;
         }
         else
         {
            // remember opening directions if field can be accessed
            if (opening_direction1.isNull())
            {
               opening_direction1 = neighbor_direction;
            }
            else
            {
               opening_direction2 = neighbor_direction;
            }
         }
      }
      else
      {
         reachable_neighbours--;
      }
   }

   // there's just one opening
   if (reachable_neighbours == 1)
   {
      // we found the dead end's end :)
      // this end position is stored at the very end of the function
      // so it'll appear at the end of the list
      if (end.isNull())
      {
         end.setX(x);
         end.setY(y);
      }

      // dig deeper
      if (recursion_depth == 1)
      {
         // go into opposite direction
         std::vector<Point> sub =
            analyzeDeadEnd(x + opening_direction1.x(), y + opening_direction1.y(), end, recursion_depth, opening_direction1);

         points.insert(points.end(), sub.begin(), sub.end());
      }
   }

   // there are two openings
   else if (reachable_neighbours == 2)
   {
      if (recursion_depth == 1)
      {
         // both direction must be the opposite of each other
         if (-opening_direction1 == opening_direction2)
         {
            // first go
            std::vector<Point> sub1 =
               analyzeDeadEnd(x + opening_direction1.x(), y + opening_direction1.y(), end, recursion_depth, opening_direction1);

            points.insert(points.end(), sub1.begin(), sub1.end());

            std::vector<Point> sub2 =
               analyzeDeadEnd(x + opening_direction2.x(), y + opening_direction2.y(), end, recursion_depth, opening_direction2);

            points.insert(points.end(), sub2.begin(), sub2.end());
         }
      }
      else
      {
         bool pointing_into_same_direction = false;
         bool pointing_back = false;

         // one must be the same direction we came from
         pointing_into_same_direction = (opening_direction1 == direction || opening_direction2 == direction);

         // one opening must be pointing into the direction we came from
         if (pointing_into_same_direction)
         {
            pointing_back =
               ((-opening_direction1.x() == direction.x() && -opening_direction1.y() == direction.y()) ||
                (-opening_direction2.x() == direction.x() && -opening_direction2.y() == direction.y()));
         }

         // if that is the case, we can continue recursion into the direction
         if (pointing_into_same_direction && pointing_back)
         {
            points.push_back(Point(x, y));

            std::vector<Point> sub = analyzeDeadEnd(x + direction.x(), y + direction.y(), end, recursion_depth, direction);

            points.insert(points.end(), sub.begin(), sub.end());
         }
      }
   }

   // we're done
   if (recursion_depth == 1 && !points.empty())
   {
      // this is no dead end
      if (end.isNull())
      {
         points.clear();
      }
      else
      {
         // "end" goes to front
         points.insert(points.begin(), end);

         int m = static_cast<int>(points.size());
         if (points.size() > 1)
         {
            // add dead end's opening
            Point other = points.at(1);

            opening_direction1.setX((other.x() > end.x()) ? 1 : (other.x() < end.x()) ? -1 : 0);

            opening_direction1.setY((other.y() > end.y()) ? 1 : (other.y() < end.y()) ? -1 : 0);
         }

         // "opening" goes to back
         points.push_back(Point(end.x() + m * opening_direction1.x(), end.y() + m * opening_direction1.y()));
      }
   }

   return points;
}

/*!
   \param x x position to check
   \param y y position to check
*/
bool ProtoBot::evaluateDeadEndSituation(int x, int y)
{
   bool safe_to_go_there = true;

   Point end;
   std::vector<Point> points = analyzeDeadEnd(x, y, end);

   // the position is actually located within a dead end
   if (!points.empty())
   {
      // check if there's an enemy in the area around opening
      Point opening = points.back();
      std::vector<BotPlayerInfo*> enemies = getEnemies();
      for (BotPlayerInfo* enemy : enemies)
      {
         if (!enemy->isKilled())
         {
            Point e = toField(enemy->getX(), enemy->getY());

            // yup. an enemy is around.
            if ((e - opening).manhattanLength() < 2)
            {
               safe_to_go_there = false;
               break;
            }
         }
      }
   }

   return safe_to_go_there;
}

void ProtoBot::markHazardousDeadEnds()
{
   int x = getXField();
   int y = getYField();

   bool mark_hazardous = false;

   Point end;
   std::vector<Point> points = analyzeDeadEnd(x, y, end);

   // the position is actually located within a dead end
   if (!points.empty())
   {
      // check if there's an enemy in the area around opening
      Point opening = points.back();
      std::vector<BotPlayerInfo*> enemies = getEnemies();

      // the dead end is small (just 1 or 2 fields)
      if (points.size() <= 3)
      {
         for (BotPlayerInfo* enemy : enemies)
         {
            if (!enemy->isKilled())
            {
               Point e = toField(enemy->getX(), enemy->getY());

               // yup. an enemy is around.
               if ((e - opening).manhattanLength() < 2)
               {
                  mark_hazardous = true;
               }
            }
         }
      }
   }

   if (mark_hazardous)
   {
      // we skip the opening
      for (size_t i = 0; i + 1 < points.size(); i++)
      {
         setScore(points[i].x(), points[i].y(), -1);
      }
   }
}
