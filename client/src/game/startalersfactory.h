#pragma once

/// \brief bonus-pickup coin/star burst, shown when a player collects an extra.

#include <memory>
#include <vector>

#include "math/vector.h"

// shared
#include <cstdint>
#include "constants.h"

class StarTalersFactory
{
public:
   StarTalersFactory();
   ~StarTalersFactory();

   void initialize();
   void add(float x, float y, Constants::ExtraType extra);
   void update(float dt);
   void render();

private:
   class Burst
   {
   public:
      Burst(const Vector& field_position, const Vector& color);
      ~Burst();

      void update(float dt);
      void render(int field_param, int color_param, int time_param);
      bool isElapsed() const;

   private:
      uint32_t _vertex_buffer = 0;
      uint32_t _index_buffer = 0;
      int _index_count = 0;
      float _time = 0.0f;
      Vector _field_position;
      Vector _color;
   };

   const Vector& getColor(Constants::ExtraType extra) const;

   std::vector<std::unique_ptr<Burst>> _bursts;

   uint32_t _shader = 0;
   uint32_t _texture_id = 0;
   int _field_param = -1;
   int _color_param = -1;
   int _time_param = -1;
   int _camera_param = -1;
   int _texture_param = -1;

   Vector _color_bomb;
   Vector _color_flame;
   Vector _color_speedup;
   Vector _color_kick;
   Vector _color_default;
};
