#pragma once

#include <cstdint>
#include <span>
#include <vector>

// the two short extra flourishes: a glowing frustum growing out of the floor when an extra is
// revealed, and an expanding ring on the floor when an extra is destroyed
class ExtraAnimations
{
public:
   ExtraAnimations();
   ~ExtraAnimations();

   ExtraAnimations(const ExtraAnimations&) = delete;
   ExtraAnimations& operator=(const ExtraAnimations&) = delete;

   void addReveal(int32_t x, int32_t y);
   void addDestroyed(int32_t x, int32_t y);
   void clear();

   //! \param dt game ticks since the last frame (62.5 per second)
   void render(float dt);

private:
   struct Reveal
   {
      int32_t x = 0;
      int32_t y = 0;
      float elapsed = 0.0f;
   };

   struct Destroyed
   {
      float x = 0.0f;
      float y = 0.0f;
      float start_time = 0.0f;
   };

   void renderReveals(float dt);
   void renderDestroyed();
   void drawTriangles(std::span<const float> vertices);

   std::vector<Reveal> _reveals;
   std::vector<Destroyed> _destroyed;

   uint32_t _frustum_texture = 0;
   uint32_t _ring_texture = 0;
   uint32_t _vertex_buffer = 0;

   uint32_t _reveal_shader = 0;
   int32_t _reveal_color_param = -1;
   int32_t _reveal_scroll_param = -1;
   int32_t _reveal_texture_param = -1;
};
