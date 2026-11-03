#include "learnedbotweights.h"

#include <algorithm>
#include <format>
#include <fstream>
#include <sstream>

namespace
{
constexpr std::array<std::string_view, LearnedBotWeights::ParameterCount> parameter_names = {
   "goal.distance",
   "goal.extra",
   "goal.stones",
   "goal.enemy_in_line",
   "goal.enemy_distance",
   "goal.open_neighbors",
   "goal.hazard_nearby",
   "goal.previous_goal",
   "goal.current_field",
   "goal.enemy_trapped",
   "bomb.bias",
   "bomb.stones",
   "bomb.enemy_in_line",
   "bomb.enemy_distance",
   "bomb.bombs_left",
   "bomb.at_goal",
   "bomb.enemy_trapped",
   "bomb.flames",
   "kick.bias",
   "kick.enemy_in_line",
   "kick.enemy_trapped",
   "kick.slide",
   "kick.fuse_left",
};
}  // namespace

LearnedBotWeights LearnedBotWeights::defaults()
{
   LearnedBotWeights weights;

   weights._goal[GoalDistance] = -1.0f;
   weights._goal[GoalExtra] = 3.0f;
   weights._goal[GoalStones] = 1.0f;
   weights._goal[GoalEnemyInLine] = 2.0f;
   weights._goal[GoalEnemyDistance] = -0.3f;
   weights._goal[GoalOpenNeighbors] = 0.2f;
   weights._goal[GoalHazardNearby] = -0.5f;
   weights._goal[GoalPreviousGoal] = 0.3f;
   weights._goal[GoalCurrentField] = 0.1f;
   weights._goal[GoalEnemyTrapped] = 2.0f;

   weights._bomb[BombBias] = -0.5f;
   weights._bomb[BombStones] = 2.5f;
   weights._bomb[BombEnemyInLine] = 2.0f;
   weights._bomb[BombEnemyDistance] = 0.0f;
   weights._bomb[BombBombsLeft] = 0.3f;
   weights._bomb[BombAtGoal] = 0.5f;
   weights._bomb[BombEnemyTrapped] = 3.0f;
   weights._bomb[BombFlames] = 0.0f;

   weights._kick[KickBias] = -1.0f;
   weights._kick[KickEnemyInLine] = 2.0f;
   weights._kick[KickEnemyTrapped] = 1.0f;
   weights._kick[KickSlide] = 0.0f;
   weights._kick[KickFuseLeft] = -0.5f;

   return weights;
}

std::array<float, LearnedBotWeights::ParameterCount> LearnedBotWeights::toParameters() const
{
   std::array<float, ParameterCount> parameters{};
   std::ranges::copy(_goal, parameters.begin());
   std::ranges::copy(_bomb, parameters.begin() + GoalFeatureCount);
   std::ranges::copy(_kick, parameters.begin() + GoalFeatureCount + BombFeatureCount);
   return parameters;
}

LearnedBotWeights LearnedBotWeights::fromParameters(std::span<const float, ParameterCount> parameters)
{
   LearnedBotWeights weights;
   std::ranges::copy(parameters.first<GoalFeatureCount>(), weights._goal.begin());
   std::ranges::copy(parameters.subspan<GoalFeatureCount, BombFeatureCount>(), weights._bomb.begin());
   std::ranges::copy(parameters.last<KickFeatureCount>(), weights._kick.begin());
   return weights;
}

std::optional<LearnedBotWeights> LearnedBotWeights::load(const std::string& path)
{
   std::ifstream file(path);

   if (!file.is_open())
   {
      return std::nullopt;
   }

   auto parameters = defaults().toParameters();
   std::string line;

   while (std::getline(file, line))
   {
      std::istringstream words(line);
      std::string name;
      float value = 0.0f;

      if (!(words >> name >> value) || name.starts_with('#'))
      {
         continue;
      }

      const auto it = std::ranges::find(parameter_names, name);

      if (it != parameter_names.end())
      {
         parameters[static_cast<size_t>(it - parameter_names.begin())] = value;
      }
   }

   return fromParameters(parameters);
}

bool LearnedBotWeights::write(const std::string& path) const
{
   std::ofstream file(path);

   if (!file.is_open())
   {
      return false;
   }

   const auto parameters = toParameters();

   for (int i = 0; i < ParameterCount; i++)
   {
      file << std::format("{} {:.5f}\n", parameter_names[static_cast<size_t>(i)], parameters[static_cast<size_t>(i)]);
   }

   return file.good();
}

std::string_view LearnedBotWeights::parameterName(int index)
{
   return parameter_names[static_cast<size_t>(index)];
}
