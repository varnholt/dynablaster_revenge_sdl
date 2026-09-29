#include "bot.h"

// bot
#include "botaction.h"
#include "botconstants.h"
#include "botmap.h"
#include "botplayerinfo.h"
#include "botwalkaction.h"

// Qt
#include "logging.h"

// cmath
#include <math.h>

#include <algorithm>
#include <unordered_set>

// defines
#define MIN_QUEUE_CHECK_SIZE 30

//-----------------------------------------------------------------------------
/*!
   \param parent parent object
*/
Bot::Bot()
    : _bot_state(BotStateDead),
      _bot_map(0),
      _player_info(0),
      _x(0.0f),
      _y(0.0f),
      _x_field(0.0f),
      _y_field(0.0f),
      _id(-1),
      _bot_keys_pressed(0),
      _decision_required(false),
      _action_required(false),
      _transiterate_target_x(0),
      _transiterate_target_y(0),
      _player_position_valid(false)
{
}

//-----------------------------------------------------------------------------
/*!
 */
Bot::~Bot()
{
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::startTicking()
{
   if (!_tick_timer.isActive())
   {
      _tick_timer.timeoutSignal.connect([this]() { tick(); });
      _tick_timer.start(100);
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::tick()
{
   // only decide/act once actually in a round - invalidate() is called from the real state
   // transitions (idle()/die()) below, not from here. Calling it on every single inactive tick
   // (as this used to) raced the one-time spawn position sync that arrives while still joining/
   // waiting for the round to start: whichever won the race left the bot permanently stuck once
   // wakeUp() flipped it active, since the server only pushes a fresh position reactively (after
   // the bot itself moves) - a deadlock, not just a race.
   if (!isActive())
   {
      return;
   }

   if (isValid())
   {
      think();
      decide();
      act();
   }

   syncSignal();
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if active
*/
bool Bot::isActive()
{
   return (_bot_state == BotStateActive);
}

//-----------------------------------------------------------------------------
/*!
   \param botmap bot map
*/
void Bot::setBotMap(BotMap* botmap)
{
   _bot_map = botmap;
}

//-----------------------------------------------------------------------------
/*!
   \param info player info ptr
*/
void Bot::setPlayerInfo(BotPlayerInfo* info)
{
   _player_info = info;
}

//-----------------------------------------------------------------------------
/*!
   \return player info ptr
*/
BotPlayerInfo* Bot::getPlayerInfo() const
{
   return _player_info;
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::think()
{
   for (BotOption* option : _options)
   {
      delete option;
   }

   _options.clear();
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::decide()
{
   _actions.clear();
   BotOption* best_option = 0;
   int maxscore = INT_MIN;

   for (BotOption* option : _options)
   {
      if (option->getScore() > maxscore)
      {
         maxscore = option->getScore();
         best_option = option;
      }

      // at the moment there's no option that is combinable
      if (option->isCombinable())
      {
         _actions.push_back(option->getAction());
      }
   }

   if (best_option)
   {
      // do not execute an action twice
      if (std::find(_actions.begin(), _actions.end(), best_option->getAction()) == _actions.end())
      {
         _actions.push_back(best_option->getAction());
      }
   }
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::act()
{
   for (BotAction* action : _actions)
   {
      switch (action->getActionType())
      {
         case BotAction::ActionBomb:
         {
            if (DEBUG_EXECUTED_ACTIONS)
               qDebug("Bot::act(): BotAction::ActionBomb:");

            bombSignal();
            break;
         }

         case BotAction::ActionWalk:
         {
            if (DEBUG_EXECUTED_ACTIONS)
               qDebug("Bot::act(): BotAction::ActionWalk:");

            walkSignal(((BotWalkAction*)action)->getWalkKeys());
            break;
         }

         case BotAction::ActionIdle:
         default:
            if (DEBUG_EXECUTED_ACTIONS)
               qDebug("Bot::act(): BotAction::ActionIdle:");

            walkSignal(0);
            break;
      }
   }
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
*/
void Bot::updatePlayerId(int id)
{
   _id = id;
}

//-----------------------------------------------------------------------------
/*!
   \param id player id
   \param x x position
   \param y y position
*/
void Bot::updatePlayerPosition(int id, float x, float y, float /*angle*/)
{
   if (id == _id)
   {
      _x = x;
      _y = y;

      _x_field = floor(x);
      _y_field = floor(y);

      setPlayerPositionValid(true);
   }
}

//-----------------------------------------------------------------------------
/*!
   \return x position
*/
float Bot::getX() const
{
   return _x;
}

//-----------------------------------------------------------------------------
/*!
   \return y position
*/
float Bot::getY() const
{
   return _y;
}

//-----------------------------------------------------------------------------
/*!
   \return x field
*/
int Bot::getXField()
{
   return _x_field;
}

//-----------------------------------------------------------------------------
/*!
   \return y field
*/
int Bot::getYField()
{
   return _y_field;
}

//-----------------------------------------------------------------------------
/*!
  \param width map width
  \param height map height
  \return map
*/
BotMap* Bot::createMap(int width, int height)
{
   return new BotMap(width, height);
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::wakeUp()
{
   // clear bot state for next round
   reset();

   setState(BotStateActive);
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::idle()
{
   // stale position data from the round that just ended shouldn't be trusted until a fresh
   // sync arrives for the next one.
   invalidate();
   setState(BotStateIdle);
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::die()
{
   invalidate();
   setState(BotStateDead);
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::extraShake(int)
{
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::markHazardousTemporary(int /*x*/, int /*y*/, int /*ms*/, int /*field_count*/)
{
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::bombKicked(int /*start_x*/, int /*start_y*/, Constants::Direction, int /*flames*/)
{
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if action is required
*/
bool Bot::isActionRequired()
{
   return _action_required;
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::reset()
{
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::cleanUpBot()
{
}

//-----------------------------------------------------------------------------
/*!
   \param state bot state
*/
void Bot::setState(BotState state)
{
   _bot_state = state;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if field has been reached
*/
bool Bot::isFieldReached()
{
   qFatal("Bot::isFieldReached(): rebel without a cause");

   bool reached = false;

   reached =
      (fabs(_x - ((float)_transiterate_target_x + 0.5f)) < FIELD_REACHED_PRECISION &&
       fabs(_y - ((float)_transiterate_target_y + 0.5f)) < FIELD_REACHED_PRECISION);

   return reached;
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::invalidate()
{
   setPlayerPositionValid(false);
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if player data is valid
*/
bool Bot::isValid() const
{
   return isPlayerPositionValid();
}

//-----------------------------------------------------------------------------
/*!
   \param valid player postion valid flag
*/
void Bot::setPlayerPositionValid(bool valid)
{
   _player_position_valid = valid;
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if player position is valid
*/
bool Bot::isPlayerPositionValid() const
{
   return _player_position_valid;
}

//-----------------------------------------------------------------------------
/*!
   \param keys_pressed bot's keys pressed
*/
void Bot::setBotKeysPressed(int8_t keys_pressed)
{
   _bot_keys_pressed = keys_pressed;
}

//-----------------------------------------------------------------------------
/*!
   \return bot's keys pressed
*/
int8_t Bot::getBotKeysPressed() const
{
   return _bot_keys_pressed;
}

//-----------------------------------------------------------------------------
/*!
 */
void Bot::updatePositionQueue()
{
   Point p(getXField(), getYField());

   if (!_position_queue.empty())
   {
      if (_position_queue.back() != p)
      {
         _position_queue.push_back(p);
      }

      while (_position_queue.size() > MIN_QUEUE_CHECK_SIZE)
      {
         _position_queue.pop_front();
      }
   }
   else
   {
      _position_queue.push_back(p);
   }
}

//-----------------------------------------------------------------------------
/*!
   \return \c true if position queue recurs
*/
bool Bot::isPositionQueueRecurrent() const
{
   bool recurrent = false;

   if (_position_queue.size() >= MIN_QUEUE_CHECK_SIZE)
   {
      std::unordered_set<Point> points;

      for (const Point& p : _position_queue)
      {
         points.insert(p);
      }

      if (points.size() <= 3)
      {
         qWarning(
            "Bot::checkPositionQueueForRecurrence(): "
            "bot positioning recurs"
         );

         recurrent = true;
      }
   }

   return recurrent;
}

//-----------------------------------------------------------------------------
/*!
   \param config reference to server configuration
*/
void Bot::setServerConfiguration(const ServerConfiguration& config)
{
   _server_configuration = config;
}

//-----------------------------------------------------------------------------
/*!
   \return reference to server configuration
*/
const ServerConfiguration& Bot::getServerConfiguration() const
{
   return _server_configuration;
}

//-----------------------------------------------------------------------------
/*!
   \return walk keys
*/
int8_t Bot::computeWalkKeys() const
{
   int8_t keys_pressed = 0;
   float field_center = 0.5f;

   if (_x - field_center < _transiterate_target_x)
      keys_pressed |= Constants::KeyRight;
   else if (_x - field_center > _transiterate_target_x)
      keys_pressed |= Constants::KeyLeft;

   if (_y - field_center < _transiterate_target_y)
      keys_pressed |= Constants::KeyDown;
   else if (_y - field_center > _transiterate_target_y)
      keys_pressed |= Constants::KeyUp;

   return keys_pressed;
}
