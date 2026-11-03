#include "learnedbot.h"

// ai
#include "botbombaction.h"
#include "botmap.h"
#include "botoption.h"
#include "botplayerinfo.h"

// shared
#include "mapitem.h"

#include <algorithm>
#include <deque>
#include <limits>

namespace
{
constexpr int max_goal_distance = 12;

template <size_t count>
float dot(const std::array<float, count>& weights, const std::array<float, count>& features)
{
   float sum = 0.0f;

   for (size_t i = 0; i < count; i++)
   {
      sum += weights[i] * features[i];
   }

   return sum;
}

const std::array<Point, 4> directions = {Point(0, -1), Point(0, 1), Point(-1, 0), Point(1, 0)};
}  // namespace

LearnedBot::LearnedBot(const PlayerInfoMap& player_info_map, std::shared_ptr<const LearnedBotWeights> weights)
    : ProtoBot(player_info_map), _weights(std::move(weights))
{
}

void LearnedBot::reset()
{
   ProtoBot::reset();
   _previous_goal = Point(-1, -1);
}

void LearnedBot::updateDistances()
{
   const int width = _bot_map->getWidth();
   const int height = _bot_map->getHeight();

   _distances.assign(static_cast<size_t>(width * height), -1);
   _parents.assign(static_cast<size_t>(width * height), -1);

   const int start = getYField() * width + getXField();
   _distances[static_cast<size_t>(start)] = 0;

   std::deque<int> open{start};

   while (!open.empty())
   {
      const int index = open.front();
      open.pop_front();

      const int distance = _distances[static_cast<size_t>(index)];

      if (distance >= max_goal_distance)
      {
         continue;
      }

      for (const Point& direction : directions)
      {
         const int x = index % width + direction.x();
         const int y = index / width + direction.y();

         if (x < 0 || y < 0 || x >= width || y >= height)
         {
            continue;
         }

         const int next = y * width + x;

         // scoreFields() left reachable safe fields at 1 or above, hazardous ones below 0
         if (_distances[static_cast<size_t>(next)] >= 0 || getScore(x, y) <= 0 || _bot_map->isPositionBlocked(x, y))
         {
            continue;
         }

         _distances[static_cast<size_t>(next)] = distance + 1;
         _parents[static_cast<size_t>(next)] = index;
         open.push_back(next);
      }
   }
}

void LearnedBot::markWayTo(const Point& goal)
{
   const int width = _bot_map->getWidth();
   const int start = getYField() * width + getXField();

   // the goal ends up highest, each step back one lower, so every step leads uphill
   int index = goal.y() * width + goal.x();
   int score = 2 + _distances[static_cast<size_t>(index)];

   while (index >= 0 && index != start)
   {
      setScore(index % width, index / width, score--);
      index = _parents[static_cast<size_t>(index)];
   }
}

bool LearnedBot::isEnemyTrapped(const Point& bomb, int flames) const
{
   const int width = _bot_map->getWidth();
   const int height = _bot_map->getHeight();

   // the new bomb's flames, stopping at the first item like the real explosion
   std::vector<bool> burning(static_cast<size_t>(width * height), false);
   burning[static_cast<size_t>(bomb.y() * width + bomb.x())] = true;

   std::vector<Point> hit_enemies;

   for (const Point& direction : directions)
   {
      for (int i = 1; i <= flames; i++)
      {
         const int x = bomb.x() + i * direction.x();
         const int y = bomb.y() + i * direction.y();

         if (x < 0 || y < 0 || x >= width || y >= height || _bot_map->getItem(x, y))
         {
            break;
         }

         burning[static_cast<size_t>(y * width + x)] = true;

         if (std::ranges::find(_enemy_positions, Point(x, y)) != _enemy_positions.end())
         {
            hit_enemies.push_back(Point(x, y));
         }
      }
   }

   // an enemy is trapped if no field within a few steps is safe from this bomb and all others
   constexpr int escape_steps = 6;

   for (const Point& enemy : hit_enemies)
   {
      std::vector<int> distance(static_cast<size_t>(width * height), -1);
      std::deque<Point> open{enemy};
      distance[static_cast<size_t>(enemy.y() * width + enemy.x())] = 0;
      bool escapes = false;

      while (!open.empty() && !escapes)
      {
         const Point p = open.front();
         open.pop_front();
         const int d = distance[static_cast<size_t>(p.y() * width + p.x())];

         for (const Point& direction : directions)
         {
            const int x = p.x() + direction.x();
            const int y = p.y() + direction.y();
            const size_t index = static_cast<size_t>(y * width + x);

            if (x < 0 || y < 0 || x >= width || y >= height || distance[index] >= 0 || (x == bomb.x() && y == bomb.y()) ||
                _bot_map->isPositionBlocked(x, y))
            {
               continue;
            }

            if (!burning[index] && getScore(x, y) >= 0)
            {
               escapes = true;
               break;
            }

            if (d + 1 < escape_steps)
            {
               distance[index] = d + 1;
               open.push_back(Point(x, y));
            }
         }
      }

      if (!escapes)
      {
         return true;
      }
   }

   return false;
}

void LearnedBot::scoreGoals()
{
   // ProtoBot already marked the way out if the current field must be left
   if (_scoring_current_hazardous)
   {
      return;
   }

   updateDistances();

   const BotPlayerInfo& info = getPlayerInfo();
   const int flames = info.getFlameCount();
   const int width = _bot_map->getWidth();
   const int height = _bot_map->getHeight();
   const Point current(getXField(), getYField());

   const int placed_bombs = static_cast<int>(_bot_map->getBombs(_id).size());
   const bool bomb_at_hand = !isNoBombInfectionActive() && info.getBombCount() > placed_bombs;

   _enemy_positions = getLivingEnemyPositions();
   const std::vector<int> stones_to_be_bombed = _bot_map->getStonesToBeBombedMap();

   const auto nearest_enemy = [this](const Point& p)
   {
      int nearest = std::numeric_limits<int>::max();

      for (const Point& enemy : _enemy_positions)
      {
         nearest = std::min(nearest, (enemy - p).manhattanLength());
      }

      return std::min(1.0f, static_cast<float>(nearest) / 10.0f);
   };

   const auto stones_hit = [&](const Point& p)
   {
      const int stones = _bot_map->getStoneCountAroundPoint(p.x(), p.y(), flames) + stones_to_be_bombed[static_cast<size_t>(p.y() * width + p.x())];
      return static_cast<float>(std::max(0, stones)) / 4.0f;
   };

   Point goal = current;
   float best_score = std::numeric_limits<float>::lowest();

   for (int y = 0; y < height; y++)
   {
      for (int x = 0; x < width; x++)
      {
         const int distance = _distances[static_cast<size_t>(y * width + x)];

         if (distance < 0)
         {
            continue;
         }

         const Point p(x, y);
         int open_neighbors = 0;
         int hazardous_neighbors = 0;

         for (const Point& direction : directions)
         {
            const int nx = x + direction.x();
            const int ny = y + direction.y();

            if (nx < 0 || ny < 0 || nx >= width || ny >= height || _bot_map->isPositionBlocked(nx, ny))
            {
               continue;
            }

            if (getScore(nx, ny) < 0)
            {
               hazardous_neighbors++;
            }
            else
            {
               open_neighbors++;
            }
         }

         const auto& item = _bot_map->getItem(x, y);

         std::array<float, LearnedBotWeights::GoalFeatureCount> features{};
         features[LearnedBotWeights::GoalDistance] = static_cast<float>(distance) / 8.0f;
         features[LearnedBotWeights::GoalExtra] = (item && item->getType() == MapItem::Extra) ? 1.0f : 0.0f;
         features[LearnedBotWeights::GoalStones] = bomb_at_hand ? stones_hit(p) : 0.0f;
         features[LearnedBotWeights::GoalEnemyInLine] =
            (bomb_at_hand && _bot_map->isBombDropDeadly(x, y, flames, _enemy_positions)) ? 1.0f : 0.0f;
         features[LearnedBotWeights::GoalEnemyDistance] = nearest_enemy(p);
         features[LearnedBotWeights::GoalOpenNeighbors] = static_cast<float>(open_neighbors) / 4.0f;
         features[LearnedBotWeights::GoalHazardNearby] = static_cast<float>(hazardous_neighbors) / 4.0f;
         features[LearnedBotWeights::GoalPreviousGoal] = (p == _previous_goal) ? 1.0f : 0.0f;
         features[LearnedBotWeights::GoalCurrentField] = (p == current) ? 1.0f : 0.0f;
         features[LearnedBotWeights::GoalEnemyTrapped] =
            (features[LearnedBotWeights::GoalEnemyInLine] > 0.0f && isEnemyTrapped(p, flames)) ? 1.0f : 0.0f;

         const float score = dot(_weights->_goal, features);

         if (score > best_score)
         {
            best_score = score;
            goal = p;
         }
      }
   }

   _previous_goal = goal;

   if (goal != current)
   {
      markWayTo(goal);
   }

   if (!bomb_at_hand)
   {
      return;
   }

   std::array<float, LearnedBotWeights::BombFeatureCount> features{};
   features[LearnedBotWeights::BombBias] = 1.0f;
   features[LearnedBotWeights::BombStones] = stones_hit(current);
   features[LearnedBotWeights::BombEnemyInLine] = _bot_map->isBombDropDeadly(current.x(), current.y(), flames, _enemy_positions) ? 1.0f : 0.0f;
   features[LearnedBotWeights::BombEnemyDistance] = nearest_enemy(current);
   features[LearnedBotWeights::BombBombsLeft] = static_cast<float>(info.getBombCount() - placed_bombs) / static_cast<float>(info.getBombCount());
   features[LearnedBotWeights::BombAtGoal] = (goal == current) ? 1.0f : 0.0f;
   features[LearnedBotWeights::BombEnemyTrapped] =
      (features[LearnedBotWeights::BombEnemyInLine] > 0.0f && isEnemyTrapped(current, flames)) ? 1.0f : 0.0f;
   features[LearnedBotWeights::BombFlames] = static_cast<float>(flames) / 8.0f;

   if (dot(_weights->_bomb, features) > 0.0f && isSafeEscapePossible())
   {
      auto option = std::make_unique<BotOption>();
      option->setScore(2);
      option->setAction(std::make_unique<BotBombAction>());
      _options.push_back(std::move(option));
      _scoring_bomb = true;
   }
}
