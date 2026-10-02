#include "stickcalibration.h"

#include <cstdlib>

std::optional<std::pair<int32_t, int32_t>> StickCalibration::process(const Axes& axes, int32_t threshold)
{
   Axes deflection{};
   bool moved = false;

   for (int32_t i = 0; i < axis_count; i++)
   {
      if (std::abs(axes[i]) > threshold)
      {
         deflection[i] = axes[i];
         moved = true;
      }
   }

   if (moved)
   {
      _samples.push_back(deflection);
      while (_samples.size() > max_samples)
      {
         _samples.pop_front();
      }

      return std::nullopt;
   }

   // back at rest: evaluate the movement once
   const auto pair = evaluate();
   _samples.clear();
   return pair;
}

void StickCalibration::reset()
{
   _samples.clear();
}

std::optional<std::pair<int32_t, int32_t>> StickCalibration::evaluate() const
{
   if (_samples.empty())
   {
      return std::nullopt;
   }

   std::array<int64_t, axis_count> sums{};
   for (const auto& sample : _samples)
   {
      for (int32_t i = 0; i < axis_count; i++)
      {
         sums[i] += std::abs(sample[i]);
      }
   }

   // even axes are x, odd ones y
   int32_t x_axis = -1;
   int32_t y_axis = -1;
   for (int32_t i = 0; i < axis_count; i++)
   {
      int32_t& best = (i % 2) ? y_axis : x_axis;
      if (sums[i] > 0 && (best == -1 || sums[i] > sums[best]))
      {
         best = i;
      }
   }

   // a stick only pushed along one axis doesn't say which stick's other axis belongs to it
   if (x_axis == -1 || y_axis == -1)
   {
      return std::nullopt;
   }

   return std::pair{x_axis, y_axis};
}
