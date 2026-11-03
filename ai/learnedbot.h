#ifndef LEARNEDBOT_H
#define LEARNEDBOT_H

#include "learnedbotweights.h"
#include "protobot.h"

#include <memory>
#include <optional>

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

   //! ProtoBot's thinking, then a kick replaces whatever it chose
   void think() override;

   void reset() override;

private:
   //! walking distance through safe fields from the bot's field, -1 where not reachable;
   //! fills _parents to walk the way back
   void updateDistances();

   //! marks the walk from the bot's field to the goal so chooseNextField() follows it
   void markWayTo(const Point& goal);

   //! a bomb at the given field would reach an enemy who has no safe field left to run to
   bool isEnemyTrapped(const Point& bomb, int flames) const;

   //! the bomb next to the bot that is worth kicking now, if any: it has to slide further than
   //! its flames reach, so the bot is out of the blast, and arrive before it explodes
   std::optional<Point> chooseKick();

   //! noise in -1..1 for this bot, a decision ("salt") and a field, that changes every 1.5 s;
   //! scaled by the weights' randomness it makes easier levels pick worse options now and then
   float noise(uint32_t salt, int index) const;

   std::shared_ptr<const LearnedBotWeights> _weights;

   std::vector<int> _distances;
   std::vector<int> _parents;
   Point _previous_goal{-1, -1};
   std::optional<Point> _kick_target;
};

#endif  // LEARNEDBOT_H
