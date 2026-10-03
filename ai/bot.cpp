#include "bot.h"

// bot
#include "botaction.h"
#include "botconstants.h"
#include "botmap.h"
#include "botplayerinfo.h"
#include "botwalkaction.h"

#include "logging.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <unordered_set>

namespace
{
constexpr size_t MIN_QUEUE_CHECK_SIZE = 30;
}

void Bot::startTicking()
{
   if (!_tick_timer.isActive())
   {
      _tick_timer.timeoutSignal.connect([this]() { tick(); });
      _tick_timer.start(100);
   }
}

void Bot::tick()
{
   // only decide/act once actually in a round - invalidate() is called from the real state
   // transitions (idle()/die()) below, not from here. Calling it on every single inactive tick
   // would race the one-time spawn position sync that arrives while still joining/waiting for
   // the round to start: whichever won the race left the bot permanently stuck once wakeUp()
   // flipped it active, since the server only pushes a fresh position reactively (after the bot
   // itself moves) - a deadlock, not just a race.
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

/*!
   \return \c true if active
*/
bool Bot::isActive()
{
   return (_bot_state == BotState::BotStateActive);
}

/*!
   \param bot_map bot map
*/
void Bot::setBotMap(std::unique_ptr<BotMap> bot_map)
{
   _bot_map = std::move(bot_map);
}

/*!
   \return bot map
*/
BotMap& Bot::getBotMap() const
{
   return *_bot_map;
}

/*!
   \param info player info
*/
void Bot::setPlayerInfo(BotPlayerInfo& info)
{
   _player_info = info;
}

/*!
   \return player info
*/
BotPlayerInfo& Bot::getPlayerInfo() const
{
   return _player_info->get();
}

void Bot::think()
{
   _options.clear();
}

void Bot::decide()
{
   _actions.clear();
   std::optional<std::reference_wrapper<const BotOption>> best_option;
   int max_score = std::numeric_limits<int>::min();

   for (const auto& option : _options)
   {
      if (option->getScore() > max_score)
      {
         max_score = option->getScore();
         best_option = *option;
      }

      // at the moment there's no option that is combinable
      if (option->isCombinable())
      {
         _actions.push_back(option->getAction());
      }
   }

   // do not execute an action twice
   if (best_option)
   {
      BotAction& best_action = best_option->get().getAction();

      if (std::ranges::none_of(_actions, [&best_action](const BotAction& action) { return &action == &best_action; }))
      {
         _actions.push_back(best_action);
      }
   }
}

void Bot::act()
{
   for (BotAction& action : _actions)
   {
      switch (action.getActionType())
      {
         case BotAction::ActionType::ActionBomb:
         {
            if (DEBUG_EXECUTED_ACTIONS)
            {
               qDebug("Bot::act(): BotAction::ActionBomb:");
            }

            bombSignal();
            break;
         }

         case BotAction::ActionType::ActionWalk:
         {
            if (DEBUG_EXECUTED_ACTIONS)
            {
               qDebug("Bot::act(): BotAction::ActionWalk:");
            }

            walkSignal(static_cast<BotWalkAction&>(action).getWalkKeys());
            break;
         }

         case BotAction::ActionType::ActionIdle:
         default:
            if (DEBUG_EXECUTED_ACTIONS)
            {
               qDebug("Bot::act(): BotAction::ActionIdle:");
            }

            walkSignal(0);
            break;
      }
   }
}

/*!
   \param id player id
*/
void Bot::updatePlayerId(int id)
{
   _id = id;
}

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

      _x_field = static_cast<int>(std::floor(x));
      _y_field = static_cast<int>(std::floor(y));

      setPlayerPositionValid(true);
   }
}

/*!
   \return x position
*/
float Bot::getX() const
{
   return _x;
}

/*!
   \return y position
*/
float Bot::getY() const
{
   return _y;
}

/*!
   \return x field
*/
int Bot::getXField()
{
   return _x_field;
}

/*!
   \return y field
*/
int Bot::getYField()
{
   return _y_field;
}

/*!
  \param width map width
  \param height map height
  \return map
*/
std::unique_ptr<BotMap> Bot::createMap(int width, int height)
{
   return std::make_unique<BotMap>(width, height);
}

void Bot::wakeUp()
{
   // clear bot state for next round
   reset();

   setState(BotState::BotStateActive);
}

void Bot::idle()
{
   // stale position data from the round that just ended shouldn't be trusted until a fresh
   // sync arrives for the next one.
   invalidate();
   setState(BotState::BotStateIdle);
}

void Bot::die()
{
   invalidate();
   setState(BotState::BotStateDead);
}

void Bot::extraShake(int)
{
}

void Bot::markHazardousTemporary(int /*x*/, int /*y*/, int /*ms*/, int /*field_count*/)
{
}

void Bot::bombKicked(int /*start_x*/, int /*start_y*/, Constants::Direction, int /*flames*/)
{
}

/*!
   \return \c true if action is required
*/
bool Bot::isActionRequired()
{
   return _action_required;
}

void Bot::reset()
{
}

/*!
   \param state bot state
*/
void Bot::setState(BotState state)
{
   _bot_state = state;
}

/*!
   \return \c true if field has been reached
*/
bool Bot::isFieldReached()
{
   qFatal("Bot::isFieldReached(): rebel without a cause");

   return std::fabs(_x - (static_cast<float>(_transiterate_target_x) + 0.5f)) < FIELD_REACHED_PRECISION &&
          std::fabs(_y - (static_cast<float>(_transiterate_target_y) + 0.5f)) < FIELD_REACHED_PRECISION;
}

void Bot::invalidate()
{
   setPlayerPositionValid(false);
}

/*!
   \return \c true if player data is valid
*/
bool Bot::isValid() const
{
   return isPlayerPositionValid();
}

/*!
   \param valid player postion valid flag
*/
void Bot::setPlayerPositionValid(bool valid)
{
   _player_position_valid = valid;
}

/*!
   \return \c true if player position is valid
*/
bool Bot::isPlayerPositionValid() const
{
   return _player_position_valid;
}

/*!
   \param keys_pressed bot's keys pressed
*/
void Bot::setBotKeysPressed(int8_t keys_pressed)
{
   _bot_keys_pressed = keys_pressed;
}

/*!
   \return bot's keys pressed
*/
int8_t Bot::getBotKeysPressed() const
{
   return static_cast<int8_t>(_bot_keys_pressed);
}

void Bot::updatePositionQueue()
{
   const Point p(getXField(), getYField());

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

/*!
   \return \c true if position queue recurs
*/
bool Bot::isPositionQueueRecurrent() const
{
   bool recurrent = false;

   if (_position_queue.size() >= MIN_QUEUE_CHECK_SIZE)
   {
      const std::unordered_set<Point> points(_position_queue.begin(), _position_queue.end());

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

/*!
   \param config reference to server configuration
*/
void Bot::setServerConfiguration(const ServerConfiguration& config)
{
   _server_configuration = config;
}

/*!
   \return reference to server configuration
*/
const ServerConfiguration& Bot::getServerConfiguration() const
{
   return _server_configuration;
}

/*!
   \return walk keys
*/
int8_t Bot::computeWalkKeys() const
{
   int8_t keys_pressed = 0;
   const float field_center = 0.5f;

   if (_x - field_center < _transiterate_target_x)
   {
      keys_pressed |= Constants::KeyRight;
   }
   else if (_x - field_center > _transiterate_target_x)
   {
      keys_pressed |= Constants::KeyLeft;
   }

   if (_y - field_center < _transiterate_target_y)
   {
      keys_pressed |= Constants::KeyDown;
   }
   else if (_y - field_center > _transiterate_target_y)
   {
      keys_pressed |= Constants::KeyUp;
   }

   return keys_pressed;
}
