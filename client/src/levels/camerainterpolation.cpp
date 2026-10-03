#include "camerainterpolation.h"
#include "framework/globaltime.h"
#include "nodes/camera.h"
#include "playerinfo.h"

#include <algorithm>
#include <cmath>
#include <numbers>

CameraInterpolation::CameraInterpolation(CameraRef center, CameraRef upper, CameraRef lower)
    : _center(center), _upper(upper), _lower(lower), _time(GlobalTime::Instance().getTime())
{
   for (auto& position : _previous_positions)
   {
      position.set(0.5f, 0.5f);
   }

   for (auto& position : _interpolated_positions)
   {
      position.set(0.5f, 0.5f);
   }
}

void CameraInterpolation::resetPlayerPositions()
{
   _players.clear();
   _active_players = 0;
}

void CameraInterpolation::startPositionUpdate(float width, float height, float dt)
{
   _inverse_width = 1.0f / width;
   _inverse_height = 1.0f / height;
   _delta_time = dt * 0.25f;
   _active_players = 0;
   _current_position.set(0.0f, 0.0f);
}

void CameraInterpolation::addPlayerPosition(const PlayerInfo& player)
{
   float fade = 1.0f;

   const auto entry = _players.find(player.getId());
   if (entry != _players.end())
   {
      if (player.isKilled())
      {
         fade = std::max(entry->second - _delta_time, 0.0f);
         entry->second = fade;
      }
   }
   else
   {
      _players[player.getId()] = 1.0f;
   }

   if (fade > 0.0f)
   {
      const float x = player.getX() * _inverse_width;
      const float y = player.getY() * _inverse_height;
      _current_position += Vector2(x, y) * fade;
      _active_players++;
   }
}

void CameraInterpolation::endPlayerPositionUpdate()
{
   const float time_step = 1.0f / 60.0f;
   const float time = GlobalTime::Instance().getTime();

   if (_active_players > 0)
   {
      _current_position = _current_position * (1.0f / _active_players);
   }
   else
   {
      _current_position.set(0.5f, 0.5f);
   }

   while (time >= _time + time_step)
   {
      _previous_positions[2] = _previous_positions[1];
      _previous_positions[1] = _previous_positions[0];
      _previous_positions[0] = _current_position;

      // 2nd order low pass iir filter
      const Vector2 position = _previous_positions[0] * 0.00105143353171677460f + _previous_positions[1] * 0.00210286706343354920f +
                               _previous_positions[2] * 0.00105143353171677460f + _interpolated_positions[0] * 1.92916981735421360000f +
                               _interpolated_positions[1] * -0.93337555158934948000f;

      _interpolated_positions[2] = _interpolated_positions[1];
      _interpolated_positions[1] = _interpolated_positions[0];
      _interpolated_positions[0] = position;

      _time += time_step;
   }
}

Matrix CameraInterpolation::getCameraMatrix(float time, float scale)
{
   float fov = 1.0f;
   Matrix camera;

   if (_center)
   {
      Camera& center = *_center;
      center.transform(time);
      fov = 1.0f / (static_cast<float>(std::tan(center.getFOV() * 0.5)) * 0.75f);
      camera = center.getTransform();
   }

   if (_upper && _lower)
   {
      _upper->get().transform(_interpolated_positions[0].x * 16000.0f);  // 100 frame a 160 ticks
      _lower->get().transform(_interpolated_positions[0].x * 16000.0f);

      const Matrix upper = _upper->get().getTransform();
      const Matrix lower = _lower->get().getTransform();

      const Matrix blended = Matrix::blend(upper, lower, _interpolated_positions[0].y);

      if (time < 60 * 160)
      {
         // no influence of player positions
      }
      else if (time < 130 * 160)
      {
         float t = (time - 60 * 160) / (70 * 160);
         t = 0.5f - std::cos(t * std::numbers::pi_v<float>) * 0.5f;
         t = t * t;
         camera = Matrix::blend(camera, blended, t);
      }
      else
      {
         camera = blended;
      }
   }

   fov *= scale;
   camera = camera.getView() * Matrix::scale(fov, fov, 1.0f);

   return camera;
}

bool CameraInterpolation::isPlayerMapEmpty() const
{
   return _active_players == 0;
}
