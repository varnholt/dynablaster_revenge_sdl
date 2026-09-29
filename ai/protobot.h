#ifndef PROTOBOT_H
#define PROTOBOT_H

// base
#include "bot.h"

#include <map>

// shared
#include "constants.h"

// astar
#include "astarpathfinding.h"

// shared
#include "point.h"

#include <deque>
#include <vector>

// forward declarations
class BombChainReaction;
class BotCharacter;
class BotWalkAction;
class ProtoBotInsults;
class ProtoBotMemory;

class ProtoBot : public Bot
{
   public:

      //! constructor
      ProtoBot();

      //! destructor
      virtual ~ProtoBot();

      //! debug bot's path in map
      void debugPathWithMap();

      //! setter for botmap
      virtual void setBotMap(BotMap* botmap);

      //! create a map
      virtual BotMap* createMap(int width, int height);

      //! setter for player info map
      void setPlayerInfoMap(std::map<int, BotPlayerInfo *> *map);

      //! getter for protobot insult message handler
      ProtoBotInsults* getInsults();

      //! getter for extra shake ids
      const std::vector<int>& getExtraShakeIds() const;

      //! getter for debug paths flag
      bool isDebugPathsEnabled() const;

      //! setter for debug paths flag
      void setDebugPathsEnabled(bool debug);

      //! getter for debug escape paths flag
      bool isDebugEscapePathsEnabled() const;

      //! setter for debug escape paths flag
      void setDebugEscapePathsEnabled(bool value);

      //! getter for debug mapitems flag
      bool isDebugMapItemsEnabled() const;

      //! setter for debug mapitems flag
      void setDebugMapItemsEnabled(bool value);

      //! getter for debug flag
      bool isDebugScoresEnabled() const;

      //! setter for debug flag
      void setDebugScoresEnabled(bool value);

      //! getter for debug flag
      bool isDebugKeysPressedEnabled() const;

      //! setter for debug flag
      void setDebugKeysPressedEnabled(bool value);

      //! getter for debug flag
      bool isDebugPossibleActionsEnabled() const;

      //! setter for debug flag
      void setDebugPossibleActionsEnabled(bool value);

      //! getter for debug flag
      bool isDebugExecutedActionsEnabled() const;

      //! setter for debug flag
      void setDebugExecutedActionsEnabled(bool value);

      //! getter for debug flag
      bool isDebugCurrentHazardousEnabled() const;

      //! setter for debug flag
      void setDebugCurrentHazardousEnabled(bool value);

      //! getter for debug flag
      bool isDebugWalkActionEnabled() const;

      //! setter for debug flag
      void setDebugWalkActionEnabled(bool value);

      //! getter for debug flag
      bool isDebugBombDropEnabled() const;

      //! setter for debug flag
      void setDebugBombDropEnabled(bool value);

      //! setter for debug breakpoint flag
      void setDebugBreakpointEnabled(bool enabled);

      //! getter for debug breakpoint flag
      bool isDebugBreakpointEnabled() const;

      //! check if we can kick ourselves out of here
      bool isKickEscapePossible(int& x, int& y);

      //! getter for idle counter
      int getIdleCounter() const;

      //! setter for idle counter
      void setIdleCounter(int value);

      //! reset idle counter
      void resetIdleCounter();

      //! increase idle counter
      void increaseIdleCounter();


   public:

      //! setter for current position (overwritten)
      virtual void updatePlayerPosition(int id, float x, float y, float angle);

      //! overwrite extra shake handler
      virtual void extraShake(int item_id);

      //! wake up bot
      virtual void wakeUp();

      //! setter for hazardous temp milliseconds
      virtual void markHazardousTemporary(int x, int y, int ms, int field_count = 0);

      //! mark dead end hazardous in case we need to leave NOW
      virtual void markHazardousDeadEnds();

      //! make hazardous temp for bomb kicks
      virtual void bombKicked(
         int start_x,
         int start_y,
         Constants::Direction,
         int flames
      );


   protected:

      // inherited from bot

      //! compute new options
      virtual void think();

      //! base overwritten
      virtual void decide();

      //! base overwritten
      virtual void act();

      //! reset bot
      virtual void reset();

      //! cleanup stuff after destructor call
      virtual void cleanUpBot();


      // custom stuff

      //! random
      int randomize(int min, int max);

      //! find path to target
      void findPath(int target_x, int target_y);

      //! find path to target
      void findPathFromTo(int start_x, int start_y, int target_x, int target_y);

      //! clear path when done
      void clearPath();

      //! calculate score for reachable positions
      void scoreFields();

      //! score for kick
      bool isKickPossible();

      //! score for escape path
      bool updateEscapeScore();

      //! score for extras
      bool updateExtraScore();

      //! score for bomb dropping
      bool updateBombStoneScore();

      //! score for attack
      bool updateAttackScore();

      //! choose the next field to go to
      void chooseNextField();

      //! setter for remaining bomb time
      void setRemainingBombTime(int x, int y, int time);

      //! getter for remaining bomb time
      int getRemainingBombTime(int x, int y) const;

      //! setter for score at x,y
      void setScore(int x, int y, int score);

      //! multiply score by factor
      void multiplyScore(int x, int y, int factor);

      //! get score for field at x,y
      int getScore(int x, int y) const;

      //! getter for hazardous temp milliseconds
      int getHazardousTemporary(int x, int y) const;

      //! update hazardous temporary array
      void updateHazardousTemporary(int ms);

      //! reset hazardous temporary array
      void resetHazardousTemporary();

      //! check if a path is dangerous
      bool isPathHazardous(const std::vector<AStarNode*>& path) const;

      //! get the number of hazardous fields per path
      int getHazardousFieldCount(const std::vector<AStarNode*>& path) const;

      //! player is currently unable to drop bombs
      bool isNoBombInfectionActive() const;

      //! check if player infection is active
      bool isInfectionActive(Constants::SkullType skull_type) const;

      //! get a list of enemy positions
      std::vector<Point> getLivingEnemyPositions() const;

      //! get a list of future enemy positions
      std::vector<Point> getLivingEnemyFuturePositions() const;

      //! get a list of enemies
      std::vector<BotPlayerInfo *> getEnemies() const;

      //! check if a bomb drop is safe for ourself
      std::vector<Point> reachablePositionsLeft(
         int x,
         int y,
         int flames,
         const std::vector<Point>& reachable_positions
      ) const;

      // transiteration functionality

      //! create walk direction from next field
      BotWalkAction* getWalkActionInstance();

      //! debug output scores
      void debugScores();

      //! check if attack is now possible
      bool isAttackPossible();

      //! check if bomb position reached
      bool isBombStonePossible();

      //! reset scores
      void resetScores();

      //! mark hazardous fields
      void markHazardousFields();

      //! mark reachable fields
      void markReachableFields();

      //! update remaining bomb times
      void updateRemainingBombTimes();

      //! pick next field to go to
      // void pickNextField();

      //! transiteration busy flag
      // bool _transiterate_busy;

      //! setter for bomb stone position
      void setBombStonePosition(const Point &weighted_point);

      //! getter for bomb stone position
      Point getBombStonePosition() const;

      //! setter for bomb stone position
      void setBombStonePositionPrevious(const Point &previous);

      //! getter for bomb stone position
      Point getBombStonePositionPrevious() const;

      //! reset bomb stone position
      void resetBombStonePosition();

      //! reset score flags
      void resetScoringFlags();

      //! evaluate long distance
      bool evaluateLongDistance(int &next_x_field, int &next_y_field) const;

      //! check if a safe escape is possible from this point
      bool isSafeEscapePossible();

      //! mark the best field in case we're in a fucked up situation
      bool updateLeastHazardousField();

      //! check if a field is a dead end
      std::vector<Point> analyzeDeadEnd(
         int x,
         int y,
         Point& end,
         int recursion_depth = 0,
         const Point& direction = Point()
      ) const;

      //! evaluate dead end situation
      bool evaluateDeadEndSituation(int x, int y);

      //! reset field bomb times
      void resetFieldBombTimes();

      //! check if field score should be overriden
      bool overrideFieldScore();


      // bug spotting

      //! track bug issue 1
      void bugTrack1();

      //! track bug issue 2
      void bugTrack2();

      //! track bug issue 3
      void bugTrack3();


      //! a star path finding
      AStarPathFinding _path_finding;

      //! best path found
      std::vector<Point> _best_escape_path;

      //! best attack path
      std::vector<Point> _best_attack_path;

      //! score field
      int* _field_scores; // [13 * 11];

      //! remaining bomb times
      int* _field_bomb_times;

      //! temporary hazardous
      int* _hazardous_temporary;

      //! bomb drop position
      Point _bomb_stone_position;

      //! previous bomb drop position
      Point _bomb_stone_position_previous;

      //! every bot has a character that defines the action's scores
      BotCharacter* _bot_character;

      //! player info map
      std::map<int, BotPlayerInfo *>* _player_info_map;

      //! protobot insults
      ProtoBotInsults* _insults;

      //! list of enemy positions (updated by scoreForAttack)
      std::vector<Point> _enemy_positions;

      //! list of shake extras
      std::vector<int> _extra_shake_ids;

      //! direction vectors
      std::vector<Point> _directions;

      //! last target position
      Point _last_target;

      //! bomb chain reaction evaluation
      BombChainReaction* _bomb_chain_reaction;

      //! bot has a memory to remember good positions and such
      ProtoBotMemory* _memory;

      //! idle counter
      int _idle;

      std::deque<Point> _last_positions;


      // scoring
      bool _scoring_current_hazardous;
      bool _scoring_prepare_attack;
      bool _scoring_escape;
      bool _scoring_extra;
      bool _scoring_prepare_bomb_stone;
      bool _scoring_bomb;
      bool _scoring_attack_possible;
      bool _scoring_bomb_stone_possible;

      // debugging
      bool _debug_breakpoint;
      bool _debug_paths;
      bool _debug_escape_paths;
      bool _debug_map_items;
      bool _debug_scores;
      bool _debug_keys_pressed;
      bool _debug_possible_actions;
      bool _debug_executed_actions;
      bool _debug_current_hazardous;
      bool _debug_walk_action;
      bool _debug_bomb_drop;
};

#endif // PROTOBOT_H
