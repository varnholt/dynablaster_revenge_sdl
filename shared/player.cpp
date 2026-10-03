#include "player.h"

#include "playerdisease.h"
#include "playerstats.h"

Player::Player(int32_t id) : _id(static_cast<int8_t>(id))
{
}

Player::~Player() = default;

void Player::reset()
{
   _killed = false;
   _bombs_dropped_count = 0;
   _bomb_count = SERVER_DEFAULT_BOMBCOUNT;
   _flame_count = SERVER_DEFAULT_FLAMECOUNT;
   _speed = SERVER_DEFAULT_SPEED;
   _kick_enabled = false;
   _player_rotation.reset();
   _disease.reset();

   // reset key flags
   _keys_pressed = 0;
   _keys_previously_pressed = 0;
   _position_skipped_counter = 0;
   _bomb_key_locked = false;
}

void Player::setLoggedIn(bool logged_in)
{
   _logged_in = logged_in;
}

bool Player::isLoggedIn() const
{
   return _logged_in;
}

int32_t Player::getKeysPressed() const
{
   return _keys_pressed;
}

int32_t Player::getKeysPressedPreviously() const
{
   return _keys_previously_pressed;
}

void Player::setX(float x)
{
   _x = x;
}

void Player::setY(float y)
{
   _y = y;
}

float Player::getX() const
{
   return _x;
}

float Player::getY() const
{
   return _y;
}

void Player::setSpeed(float speed)
{
   _speed = speed;
}

float Player::getSpeed() const
{
   float speed = _speed;

   if (isInfected())
   {
      if (getDisease().getType() == Constants::SkullSlow)
      {
         speed = SERVER_SKULL_SPEED_MIN;
      }
      else if (getDisease().getType() == Constants::SkullFast)
      {
         speed = SERVER_SKULL_SPEED_MAX;
      }
   }

   return speed;
}

int8_t Player::getId() const
{
   return _id;
}

void Player::setKeysPressed(int32_t keys)
{
   _keys_previously_pressed = _keys_pressed;
   _keys_pressed = keys;
}

PlayerRotation& Player::getPlayerRotation()
{
   return _player_rotation;
}

void Player::setNick(const std::string& nick)
{
   _nick = nick;
}

const std::string& Player::getNick() const
{
   return _nick;
}

void Player::setBombCount(int8_t count)
{
   _bomb_count = count;
}

int8_t Player::getBombCount() const
{
   auto bomb_count = static_cast<int8_t>(_bomb_count);

   if (isInfected())
   {
      const Constants::SkullType skull_type = getDisease().getType();

      if (skull_type == Constants::SkullNoBomb)
      {
         bomb_count = 0;
      }
      else if (skull_type == Constants::SkullMinimumBomb)
      {
         bomb_count = 1;
      }
      else if (skull_type == Constants::SkullMaximumBomb)
      {
         bomb_count = 10;
      }
   }

   return bomb_count;
}

void Player::setFlameCount(int8_t count)
{
   _flame_count = count;
}

int8_t Player::getFlameCount() const
{
   auto flame_count = static_cast<int8_t>(_flame_count);

   if (isInfected())
   {
      const Constants::SkullType skull_type = getDisease().getType();

      if (skull_type == Constants::SkullMinimumBomb)
      {
         flame_count = 1;
      }
      else if (skull_type == Constants::SkullMaximumBomb)
      {
         flame_count = 10;
      }
   }

   return flame_count;
}

void Player::setBombsDroppedCount(int8_t count)
{
   _bombs_dropped_count = count;
}

int8_t Player::getBombsDroppedCount() const
{
   return static_cast<int8_t>(_bombs_dropped_count);
}

void Player::setKilled(bool killed)
{
   _killed = killed;
}

bool Player::isKilled() const
{
   return _killed;
}

int8_t Player::getBombCountDefault()
{
   return SERVER_DEFAULT_BOMBCOUNT;
}

int8_t Player::getFlameCountDefault()
{
   return SERVER_DEFAULT_FLAMECOUNT;
}

void Player::setKickEnabled(bool enabled)
{
   _kick_enabled = enabled;
}

bool Player::isKickEnabled() const
{
   return _kick_enabled;
}

int32_t Player::getPositionSkipCounter() const
{
   return _position_skipped_counter;
}

void Player::setPositionSkipCounter(int32_t count)
{
   _position_skipped_counter = count;
}

PlayerStats& Player::getOverallStats()
{
   return _overall_stats;
}

PlayerStats& Player::getRoundStats()
{
   return _round_stats;
}

void Player::setColor(Constants::Color color)
{
   _color = color;
}

Constants::Color Player::getColor() const
{
   return _color;
}

void Player::setLoadingSynchronized(bool synchronized)
{
   _loading_synchronized = synchronized;
}

bool Player::isLoadingSynchronized() const
{
   return _loading_synchronized;
}

void Player::setBot(bool bot)
{
   _bot = bot;
}

bool Player::isBot() const
{
   return _bot;
}

void Player::infect(std::unique_ptr<PlayerDisease> disease)
{
   if (_disease)
   {
      _disease->abort();
   }

   // assignment destroys whatever was previously owned
   _disease = std::move(disease);
}

bool Player::isInfected() const
{
   return _disease != nullptr;
}

bool Player::isInvincible() const
{
   return isInfected() && getDisease().getType() == Constants::SkullInvincible;
}

PlayerDisease& Player::getDisease() const
{
   return *_disease;
}

void Player::increaseKills()
{
   getOverallStats().increaseKills();
   getRoundStats().increaseKills();
}

void Player::increaseDeaths()
{
   getOverallStats().increaseDeaths();
   getRoundStats().increaseDeaths();
}

void Player::increaseWins()
{
   getOverallStats().increaseWins();
   getRoundStats().increaseWins();
}

void Player::increaseSurvivalTime(uint32_t survival_time)
{
   getOverallStats().increaseSurvivalTime(survival_time);
   getRoundStats().increaseSurvivalTime(survival_time);
}

void Player::increaseExtrasCollected()
{
   getOverallStats().increaseExtrasCollected();
   getRoundStats().increaseExtrasCollected();
}

void Player::resetStats()
{
   getOverallStats().reset();
   getRoundStats().reset();
}

void Player::increaseFlameCount()
{
   _flame_count++;
}

void Player::increaseBombCount()
{
   _bomb_count++;
}

bool Player::isBombKeyLocked() const
{
   return _bomb_key_locked;
}

void Player::setBombKeyLocked(bool processed)
{
   _bomb_key_locked = processed;
}
