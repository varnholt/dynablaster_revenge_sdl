#pragma once

#include "render/texture.h"

#include <cstdint>
#include <vector>

// blue ribbons spiralling up from a field when a player picks up the invincibility skull
class RibbonAnimationFactory
{
public:
   RibbonAnimationFactory();
   ~RibbonAnimationFactory();

   RibbonAnimationFactory(const RibbonAnimationFactory&) = delete;
   RibbonAnimationFactory& operator=(const RibbonAnimationFactory&) = delete;

   //! add a new animation at the given field position
   void add(float x, float y);
   void clear();

   //! \param dt game ticks since the last frame (62.5 per second)
   void update(float dt);

private:
   struct Ribbon
   {
      float x = 0.0f;
      float y = 0.0f;
      float time = 0.0f;
   };

   void initBuffers();
   void draw(const Ribbon& ribbon);

   std::vector<Ribbon> _ribbons;

   Texture _texture;
   uint32_t _vertex_buffer = 0;
   uint32_t _index_buffer = 0;
   int32_t _index_count = 0;

   uint32_t _shader = 0;
   int32_t _time_param = -1;
   int32_t _field_position_param = -1;
   int32_t _circle_offset_param = -1;
   int32_t _texture_param = -1;
};
