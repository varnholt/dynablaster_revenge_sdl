#pragma once

#include "math/matrix.h"
#include "math/vector2.h"

#include <array>
#include <cstdint>
#include <unordered_map>

class Camera;
class PlayerInfo;

class CameraInterpolation
{
public:
   CameraInterpolation(Camera* center, Camera* upper, Camera* lower);

   void resetPlayerPositions();
   void startPositionUpdate(float width, float height, float dt);
   void addPlayerPosition(PlayerInfo* player);
   void endPlayerPositionUpdate();
   Matrix getCameraMatrix(float time, float scale = 1.0f);
   bool isPlayerMapEmpty() const;

private:
   float _inverse_width = 0.0f;
   float _inverse_height = 0.0f;
   Camera* _center = nullptr;
   Camera* _upper = nullptr;
   Camera* _lower = nullptr;
   Vector2 _current_position{0.0f, 0.0f};
   std::array<Vector2, 3> _previous_positions;
   std::array<Vector2, 3> _interpolated_positions;
   float _time = 0.0f;
   float _delta_time = 0.0f;
   int32_t _active_players = 0;
   std::unordered_map<PlayerInfo*, float> _players;
};
