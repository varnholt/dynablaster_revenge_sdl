#ifndef LEARNEDBOTWEIGHTS_H
#define LEARNEDBOTWEIGHTS_H

#include <array>
#include <optional>
#include <span>
#include <string>
#include <string_view>

//! the numbers LearnedBot decides by: two linear scorers over small feature vectors.
//! The goal scorer rates every field the bot can safely reach, the best one becomes its goal.
//! The bomb scorer rates the bot's own field, a bomb is dropped when it scores above zero
//! (and only if an escape exists, that check stays code). Trained by dynablaster_sim --train.
struct LearnedBotWeights
{
   //! features of a field the bot could walk to, each roughly within 0..1
   enum GoalFeature
   {
      GoalDistance,       //!< walking distance / 8
      GoalExtra,          //!< an extra lies there
      GoalStones,         //!< stones a bomb there would destroy / 4, 0 without a bomb at hand
      GoalEnemyInLine,    //!< a bomb there would reach an enemy, 0 without a bomb at hand
      GoalEnemyDistance,  //!< manhattan distance to the nearest enemy / 10, at most 1
      GoalOpenNeighbors,  //!< safe walkable neighbors / 4, few means a dead end
      GoalHazardNearby,   //!< hazardous neighbors / 4
      GoalPreviousGoal,   //!< it was the goal last time, keeps the bot from dithering
      GoalCurrentField,   //!< the bot stands there
      GoalEnemyTrapped,   //!< a bomb there would reach an enemy who can't get out of its way, 0 without a bomb at hand
      GoalFeatureCount
   };

   //! features of the bot's own field when deciding to drop a bomb
   enum BombFeature
   {
      BombBias,           //!< always 1
      BombStones,         //!< stones the bomb would destroy / 4
      BombEnemyInLine,    //!< the bomb would reach an enemy
      BombEnemyDistance,  //!< manhattan distance to the nearest enemy / 10, at most 1
      BombBombsLeft,      //!< share of the bot's bombs not yet placed
      BombAtGoal,         //!< the bot stands on its goal
      BombEnemyTrapped,   //!< the bomb would reach an enemy who can't get out of its way
      BombFlames,         //!< the bot's own flame length / 8
      BombFeatureCount
   };

   static constexpr int ParameterCount = GoalFeatureCount + BombFeatureCount;

   std::array<float, GoalFeatureCount> _goal{};
   std::array<float, BombFeatureCount> _bomb{};

   //! hand-set start values that roughly follow ProtoBot's priorities
   static LearnedBotWeights defaults();

   //! all parameters in one row, goal first, for training
   std::array<float, ParameterCount> toParameters() const;

   //! inverse of toParameters()
   static LearnedBotWeights fromParameters(std::span<const float, ParameterCount> parameters);

   //! reads "name value" lines, see write(); missing names keep their default
   static std::optional<LearnedBotWeights> load(const std::string& path);

   //! writes one "name value" line per parameter
   bool write(const std::string& path) const;

   //! names as written to the file, e.g. "goal.extra"
   static std::string_view parameterName(int index);
};

#endif  // LEARNEDBOTWEIGHTS_H
