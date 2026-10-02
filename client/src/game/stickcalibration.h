#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <optional>
#include <utility>

/// \brief finds the stick a player moves (JoystickCalibration): collects the stick axes while one
/// is deflected and, once all are back at rest, picks the most moved x and y axis.
class StickCalibration
{
public:
   //! the stick axes of SDL_GamepadAxis: left x, left y, right x, right y
   static constexpr int32_t axis_count = 4;
   using Axes = std::array<int16_t, axis_count>;

   /// \brief feeds one reading, values within the threshold count as rest
   /// \return the x and y axis once a movement ended
   std::optional<std::pair<int32_t, int32_t>> process(const Axes& axes, int32_t threshold);

   void reset();

private:
   std::optional<std::pair<int32_t, int32_t>> evaluate() const;

   static constexpr size_t max_samples = 1000;
   std::deque<Axes> _samples;
};
