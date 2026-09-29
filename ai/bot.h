#ifndef BOT_H
#define BOT_H

// ai
#include "botoption.h"

// shared
#include "constants.h"
#include "point.h"
#include "serverconfiguration.h"
#include "signal.h"
#include "timer.h"

#include <deque>
#include <vector>

// forward declarations
class BotMap;
class BotPlayerInfo;

class Bot
{
public:
   //! bot state
   enum BotState
   {
      BotStateIdle,
      BotStateActive,
      BotStateDead
   };

   //! constructor
   Bot();

   //! destructor
   virtual ~Bot();

   //! setter for bot map
   virtual void setBotMap(BotMap* botmap);

   //! setter for player info ptr
   void setPlayerInfo(BotPlayerInfo* info);

   //! getter for player info ptr
   BotPlayerInfo* getPlayerInfo() const;

   //! getter for current position x
   float getX() const;

   //! getter for current position y
   float getY() const;

   //! getter for field x
   int getXField();

   //! getter for field y
   int getYField();

   //! create a map
   virtual BotMap* createMap(int width, int height);

   //! setter for server configuration
   void setServerConfiguration(const ServerConfiguration&);

   //! getter for server configuration
   const ServerConfiguration& getServerConfiguration() const;

   //! start ticking the bot's think/decide/act loop, once per game round
   void startTicking();

   //! setter for bot keys pressed
   void setBotKeysPressed(int8_t keys_pressed);

   //! getter for bot keys pressed
   int8_t getBotKeysPressed() const;

   //! compute walk keys
   int8_t computeWalkKeys() const;

   //! drop bomb
   Signal<> bombSignal;

   //! send walk keys
   Signal<int8_t> walkSignal;

   //! time to sync
   Signal<> syncSignal;

public:

   //! setter for current position
   virtual void updatePlayerPosition(int id, float x, float y, float angle);

   //! setter for player id
   void updatePlayerId(int id);

   //! wake up bot
   virtual void wakeUp();

   //! go to idle
   virtual void idle();

   //! die
   virtual void die();

   //! extra shake packet spotted
   virtual void extraShake(int);

   //! mark hazardous temporary
   virtual void markHazardousTemporary(int x, int y, int ms, int field_count = 0);

   //! make hazardous temp for bomb kicks
   virtual void bombKicked(int start_x, int start_y, Constants::Direction, int flames);

protected:
   // bot base functionality

   //! compute new options
   virtual void think();

   //! decide what's to do next
   virtual void decide();

   //! do some action
   virtual void act();

   //! check if an action is required
   virtual bool isActionRequired();

   //! reset bot states
   virtual void reset();

   //! delete stuff in destructor
   virtual void cleanUpBot();

   //! think/decide/act once, called every _tick_timer interval while active
   virtual void tick();

   // game state transitions

   //! check if bot is active
   bool isActive();

   //! setter for bot's state
   void setState(BotState state);

   // navigation

   //! check if field is already reached
   bool isFieldReached();

   // bot information

   //! invalidate
   void invalidate();

   //! check if player position is valid
   bool isValid() const;

   //! setter for valid flag
   void setPlayerPositionValid(bool valid);

   //! getter for valid flag
   bool isPlayerPositionValid() const;

   //! update position queue
   void updatePositionQueue();

   //! check if player's brain seems to be fused
   bool isPositionQueueRecurrent() const;

   // members

   //! bot state
   BotState _bot_state;

   //! bot map
   BotMap* _bot_map;

   //! player info ptr
   BotPlayerInfo* _player_info;

   //! bot's options
   std::vector<BotOption*> _options;

   //! bot's next action
   std::vector<BotAction*> _actions;

   //! x position
   float _x;

   //! y position
   float _y;

   //! x field
   int _x_field;

   //! y field
   int _y_field;

   //! bot id
   int _id;

   //! ticks tick() at the same ~100ms cadence the old QThread loop's msleep(100) had
   Timer _tick_timer;

   //! current walk direction
   int _bot_keys_pressed;

   //! decision required flag
   bool _decision_required;

   //! action required flag
   bool _action_required;

   // navigation

   //! transiteration target x
   int _transiterate_target_x;

   //! transiteration target y
   int _transiterate_target_y;

   //! player position is valid
   bool _player_position_valid;

   //! store last few player positions
   std::deque<Point> _position_queue;

   // server related

   //! server configuration
   ServerConfiguration _server_configuration;
};

#endif  // BOT_H
