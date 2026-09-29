#pragma once

#include "math/vector.h"
#include "render/texture.h"

#include <cstdint>

// the space level's backdrop: a scrolling starfield and a rotating, lit earth with an aura ring
class SpaceBackground
{
public:
   SpaceBackground();
   ~SpaceBackground();

   SpaceBackground(const SpaceBackground&) = delete;
   SpaceBackground& operator=(const SpaceBackground&) = delete;

   //! \param dt game ticks since the last frame (62.5 per second)
   void animate(float dt);

   //! clears the frame and draws the backdrop, restores the projection afterwards
   void draw();

private:
   void initEarth();
   void initAura();
   void drawStarField();
   void drawEarth();
   void drawAura();

   Texture _star_field_texture;
   Texture _earth_texture;
   Texture _earth_normal_texture;

   uint32_t _earth_shader = 0;
   int32_t _light_param = -1;
   int32_t _camera_param = -1;
   int32_t _cloud_move_param = -1;
   int32_t _texture_map_param = -1;
   int32_t _normal_map_param = -1;

   uint32_t _earth_vertex_buffer = 0;
   uint32_t _earth_index_buffer = 0;
   int32_t _earth_index_count = 0;

   uint32_t _aura_shader = 0;
   uint32_t _aura_vertex_buffer = 0;
   int32_t _aura_vertex_count = 0;

   uint32_t _quad_vertex_buffer = 0;

   Vector _camera = Vector(-0.15f, 0.45f, 2.0f);
   float _earth_rotation = 0.0f;
};
