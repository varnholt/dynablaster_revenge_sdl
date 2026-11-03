#ifndef LEARNEDBOT_H
#define LEARNEDBOT_H

#include "learnedbotweights.h"
#include "protobot.h"

#include <memory>

//! ProtoBot with its goal choice replaced by LearnedBotWeights: every field that can be reached
//! without crossing a hazard gets scored, the bot walks to the best one and drops a bomb when
//! the bomb scorer says so. The hazard map, escaping, the escape check before a bomb, path
//! finding and walking stay ProtoBot's, so bad weights make a dull bot, not a suicidal one.
class LearnedBot : public ProtoBot
{
public:
   LearnedBot(const PlayerInfoMap& player_info_map, std::shared_ptr<const LearnedBotWeights> weights);

protected:
   void scoreGoals() override;

   void reset() override;

private:
   //! walking distance through safe fields from the bot's field, -1 where not reachable;
   //! fills _parents to walk the way back
   void updateDistances();

   //! marks the walk from the bot's field to the goal so chooseNextField() follows it
   void markWayTo(const Point& goal);

   std::shared_ptr<const LearnedBotWeights> _weights;

   std::vector<int> _distances;
   std::vector<int> _parents;
   Point _previous_goal{-1, -1};
};

#endif  // LEARNEDBOT_H
