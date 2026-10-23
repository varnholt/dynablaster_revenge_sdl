#pragma once

#include "math/matrix.h"
#include "math/vector2.h"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>

class Camera;
class PlayerInfo;

class CameraInterpolation
{
public:
   using CameraRef = std::optional<std::reference_wrapper<Camera>>;

   CameraInterpolation(CameraRef center, CameraRef upper, CameraRef lower);

   void resetPlayerPositions();
   void startPositionUpdate(float width, float height, float dt);
   void addPlayerPosition(const PlayerInfo& player);
   void endPlayerPositionUpdate();
   Matrix getCameraMatrix(float time, float scale = 1.0f);
   bool isPlayerMapEmpty() const;

   //! the fly-in camera alone until intro_end, blended into the cameras following the players until blend_end
   void setIntroTimes(float intro_end, float blend_end);

private:
   float _inverse_width = 0.0f;
   float _inverse_height = 0.0f;
   CameraRef _center;
   CameraRef _upper;
   CameraRef _lower;
   Vector2 _current_position{0.0f, 0.0f};
   std::array<Vector2, 3> _previous_positions;
   std::array<Vector2, 3> _interpolated_positions;
   float _time = 0.0f;
   float _intro_end = 60 * 160;
   float _blend_end = 130 * 160;
   float _delta_time = 0.0f;
   int32_t _active_players = 0;
   std::unordered_map<int32_t, float> _players;  // fade per player id
};
