#pragma once

/// \brief bonus-pickup coin/star burst, shown when a player collects an extra.

#include <memory>
#include <vector>

#include "math/vector.h"

// shared
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
      unsigned int _vertex_buffer;
      unsigned int _index_buffer;
      int _index_count;
      float _time;
      Vector _field_position;
      Vector _color;
   };

   const Vector& getColor(Constants::ExtraType extra) const;

   std::vector<std::unique_ptr<Burst>> _bursts;

   unsigned int _shader;
   unsigned int _texture_id;
   int _field_param;
   int _color_param;
   int _time_param;
   int _camera_param;
   int _texture_param;

   Vector _color_bomb;
   Vector _color_flame;
   Vector _color_speedup;
   Vector _color_kick;
   Vector _color_default;
};
