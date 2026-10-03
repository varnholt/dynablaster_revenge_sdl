#include "playerinfo.h"

#include "playerdisease.h"

PlayerInfo::PlayerInfo() = default;

PlayerInfo::~PlayerInfo() = default;

void PlayerInfo::setId(int32_t id)
{
   _id = id;
}

void PlayerInfo::setColor(Constants::Color color)
{
   _color = color;
}

void PlayerInfo::setNick(const std::string& nick)
{
   _nick = nick;
}

void PlayerInfo::setPosition(float x, float y, float angle)
{
   _x = x;
   _y = y;
   _angle = angle;
}

void PlayerInfo::setPositionDelta(float delta_x, float delta_y, float delta_angle)
{
   setDeltaX(delta_x);
   setDeltaY(delta_y);
   setDeltaAngle(delta_angle);
}

float PlayerInfo::getX() const
{
   return _x;
}

float PlayerInfo::getY() const
{
   return _y;
}

float PlayerInfo::getAngle() const
{
   return _angle;
}

void PlayerInfo::setDeltaX(float value)
{
   _delta_x = value;
}

void PlayerInfo::setDeltaY(float value)
{
   _delta_y = value;
}

void PlayerInfo::setDeltaAngle(float value)
{
   _delta_angle = value;
}

int32_t PlayerInfo::getId() const
{
   return _id;
}

Constants::Color PlayerInfo::getColor() const
{
   return _color;
}

std::string PlayerInfo::getNick() const
{
   return _nick;
}

float PlayerInfo::getDeltaX() const
{
   return _delta_x;
}

float PlayerInfo::getDeltaY() const
{
   return _delta_y;
}

float PlayerInfo::getAngleDelta() const
{
   return _delta_angle;
}

void PlayerInfo::setOverallStats(const PlayerStats& stats)
{
   _overall_stats = stats;
}

PlayerStats& PlayerInfo::getOverallStats()
{
   return _overall_stats;
}

void PlayerInfo::setRoundStats(const PlayerStats& stats)
{
   _round_stats = stats;
}

PlayerStats& PlayerInfo::getRoundStats()
{
   return _round_stats;
}

void PlayerInfo::setKilled(bool killed)
{
   _killed = killed;
}

bool PlayerInfo::isKilled() const
{
   return _killed;
}

void PlayerInfo::infect(std::unique_ptr<PlayerDisease> disease)
{
   // assignment destroys whatever was previously owned
   _disease = std::move(disease);
}

bool PlayerInfo::isInfected() const
{
   return _disease != nullptr;
}

PlayerDisease& PlayerInfo::getDisease() const
{
   return *_disease;
}

int8_t PlayerInfo::getDirections() const
{
   return _directions;
}

void PlayerInfo::setDirections(int8_t directions)
{
   _directions = directions;
}
