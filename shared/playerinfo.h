#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "constants.h"
#include "playerstats.h"

class PlayerDisease;

class PlayerInfo
{
public:
   PlayerInfo();

   // out-of-line since the unique_ptr needs PlayerDisease's complete type
   ~PlayerInfo();

   void setId(int32_t id);
   void setColor(Constants::Color color);
   void setNick(const std::string& nick);

   [[nodiscard]] int32_t getId() const;
   [[nodiscard]] Constants::Color getColor() const;
   [[nodiscard]] std::string getNick() const;

   void setPosition(float x, float y, float angle);
   void setPositionDelta(float delta_x, float delta_y, float delta_angle);

   [[nodiscard]] float getX() const;
   [[nodiscard]] float getY() const;
   [[nodiscard]] float getAngle() const;

   void setDeltaX(float value);
   void setDeltaY(float value);
   void setDeltaAngle(float value);

   [[nodiscard]] float getDeltaX() const;
   [[nodiscard]] float getDeltaY() const;
   [[nodiscard]] float getAngleDelta() const;

   void setOverallStats(const PlayerStats& stats);
   [[nodiscard]] PlayerStats& getOverallStats();
   [[nodiscard]] const PlayerStats& getOverallStats() const;

   void setRoundStats(const PlayerStats& stats);
   [[nodiscard]] PlayerStats& getRoundStats();
   [[nodiscard]] const PlayerStats& getRoundStats() const;

   void setKilled(bool killed);
   [[nodiscard]] bool isKilled() const;

   void infect(std::unique_ptr<PlayerDisease> disease);
   [[nodiscard]] bool isInfected() const;
   // only valid while isInfected()
   [[nodiscard]] PlayerDisease& getDisease() const;

   [[nodiscard]] int8_t getDirections() const;
   void setDirections(int8_t directions);

protected:
   int32_t _id = -1;
   Constants::Color _color = Constants::ColorWhite;
   PlayerStats _overall_stats;
   PlayerStats _round_stats;
   std::string _nick;
   float _x = 0.0f;
   float _y = 0.0f;
   float _angle = 0.0f;
   float _delta_x = 0.0f;
   float _delta_y = 0.0f;
   float _delta_angle = 0.0f;
   bool _killed = false;
   std::unique_ptr<PlayerDisease> _disease;
   int8_t _directions = 0;
};
