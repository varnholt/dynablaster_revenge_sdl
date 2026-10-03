#pragma once

/// \brief GLES3 port of client/src/game/playerdeatheffect.cpp - see project memory for the full
/// GPGPU-particle-simulation background. Interface unchanged from the original.

#include "image/image.h"

#include <cstdint>
#include <memory>
#include <vector>

class DeathFlowFieldAnimation;
class Material;
class Vector;
class Matrix;
class FrameBuffer;

class PlayerDeathEffect
{
public:
   PlayerDeathEffect();
   ~PlayerDeathEffect();

   void clear();

   void add(Material& player_material);

   void animate(float delta);
   void render();

private:
   std::vector<std::unique_ptr<DeathFlowFieldAnimation>> _flow_animations;

   uint32_t _particle_texture_id = 0;
   uint32_t _flow_field_texture_id;

   // draws vertices as point sprites
   uint32_t _points_shader = 0;
   int _points_texture = 0;
   int _points_color_map;
   int _points_size;
   int _points_projection;

   // move vertices along flowfield
   uint32_t _flow_shader;
   int _flow_vertex_pos_texture;
   int _flow_vertex_col_texture;
   int _flow_field_texture;
   int _flow_center;
   int _flow_field_scale;
   int _flow_time_delta;

   uint32_t _flow_init_pos_shader;
   int _flow_init_pos_depth;
   int _flow_init_pos_inv_proj;

   uint32_t _flow_init_param_shader;
   int _flow_init_param_depth;
   int _flow_init_param_inv_proj;
   int _flow_init_param_center;

   // owns its own offscreen render target for capturing a dying player's color+depth (see
   // DeathFlowFieldAnimation) - this port has no MainDrawable to borrow a shared render-buffer
   // from, matching MenuDrawable's own established "own your FBO" pattern.
   std::unique_ptr<FrameBuffer> _deferred_buffer;
};
