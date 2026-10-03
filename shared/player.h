#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "constants.h"
#include "playerrotation.h"
#include "playerstats.h"

class PlayerDisease;

class Player
{
public:
   explicit Player(int32_t id);

   // out-of-line since the unique_ptr needs PlayerDisease's complete type
   ~Player();

   void reset();

   void setLoggedIn(bool logged_in);
   [[nodiscard]] bool isLoggedIn() const;

   void setKeysPressed(int32_t keys);
   [[nodiscard]] int32_t getKeysPressed() const;
   [[nodiscard]] int32_t getKeysPressedPreviously() const;

   void setX(float x);
   void setY(float y);
   [[nodiscard]] float getX() const;
   [[nodiscard]] float getY() const;

   void setSpeed(float speed);
   [[nodiscard]] float getSpeed() const;

   [[nodiscard]] int8_t getId() const;

   [[nodiscard]] PlayerRotation& getPlayerRotation();

   void setNick(const std::string& nick);
   [[nodiscard]] const std::string& getNick() const;

   void setBombCount(int8_t count);
   [[nodiscard]] int8_t getBombCount() const;

   void setFlameCount(int8_t count);
   [[nodiscard]] int8_t getFlameCount() const;

   void setBombsDroppedCount(int8_t count);
   [[nodiscard]] int8_t getBombsDroppedCount() const;

   void setKilled(bool killed);
   [[nodiscard]] bool isKilled() const;

   void setKickEnabled(bool enabled);
   [[nodiscard]] bool isKickEnabled() const;

   [[nodiscard]] static int8_t getBombCountDefault();
   [[nodiscard]] static int8_t getFlameCountDefault();

   [[nodiscard]] int32_t getPositionSkipCounter() const;
   void setPositionSkipCounter(int32_t count);

   [[nodiscard]] PlayerStats& getOverallStats();
   [[nodiscard]] PlayerStats& getRoundStats();

   void setColor(Constants::Color color);
   [[nodiscard]] Constants::Color getColor() const;

   void setLoadingSynchronized(bool synchronized);
   [[nodiscard]] bool isLoadingSynchronized() const;

   void setBot(bool bot);
   [[nodiscard]] bool isBot() const;

   void infect(std::unique_ptr<PlayerDisease> disease);
   [[nodiscard]] bool isInfected() const;
   [[nodiscard]] bool isInvincible() const;
   // only valid while isInfected()
   [[nodiscard]] PlayerDisease& getDisease() const;

   void increaseKills();
   void increaseDeaths();
   void increaseWins();
   void increaseSurvivalTime(uint32_t survival_time);
   void increaseExtrasCollected();
   void resetStats();

   void increaseFlameCount();
   void increaseBombCount();

   bool isBombKeyLocked() const;
   void setBombKeyLocked(bool processed);

private:
   int8_t _id = 0;
   bool _logged_in = false;
   float _x = 0.5f;
   float _y = 0.5f;
   std::string _nick;
   int32_t _bomb_count = SERVER_DEFAULT_BOMBCOUNT;
   int32_t _flame_count = SERVER_DEFAULT_FLAMECOUNT;
   float _speed = SERVER_DEFAULT_SPEED;
   int32_t _keys_pressed = 0;
   int32_t _keys_previously_pressed = 0;
   int32_t _bombs_dropped_count = 0;
   bool _bomb_key_locked = false;
   PlayerRotation _player_rotation;
   bool _killed = false;
   bool _kick_enabled = false;

   // number of frames since the last position packet was sent
   int32_t _position_skipped_counter = 0;

   PlayerStats _overall_stats;
   PlayerStats _round_stats;
   Constants::Color _color = Constants::ColorWhite;
   bool _loading_synchronized = false;
   bool _bot = false;
   std::unique_ptr<PlayerDisease> _disease;
};
