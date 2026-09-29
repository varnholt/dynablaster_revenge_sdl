#pragma once

#include <cstdint>
#include <memory>
#include <vector>

class InfectedFlowFieldAnimation;
class Material;
class Vector;
class Matrix;
class FrameBuffer;

class PlayerInfectedEffect
{
public:
   PlayerInfectedEffect();
   ~PlayerInfectedEffect();

   void clear();

   void add(Material* player_material);
   void remove(Material* player_material);

   void animate(float delta);
   void render();

private:
   struct Flow
   {
      std::unique_ptr<InfectedFlowFieldAnimation> animation;
      Material* material;
   };

   std::vector<Flow> _flow_animations;

   float _delta = 0.0f;

   uint32_t _particle_texture_id = 0;
   uint32_t _flow_field_texture_id = 0;

   uint32_t _points_shader = 0;
   int _points_texture = 0;
   int _points_size = 0;
   int _points_projection = 0;

   uint32_t _flow_update_pos_shader = 0;
   int _flow_depth_texture = 0;
   int _flow_vertex_pos_texture = 0;
   int _flow_vertex_param_texture = 0;
   int _flow_field_texture = 0;
   int _flow_center = 0;
   int _flow_field_scale = 0;
   int _flow_time_delta = 0;
   int _flow_src_rect = 0;
   int _flow_inv_proj = 0;
   int _flow_stop = 0;

   uint32_t _flow_init_pos_shader = 0;
   int _flow_init_pos_depth = 0;
   int _flow_init_pos_inv_proj = 0;
   int _flow_init_src_rect = 0;

   uint32_t _flow_init_param_shader = 0;
   int _flow_init_param_depth = 0;
   int _flow_init_param_inv_proj = 0;
   int _flow_init_param_center = 0;

   uint32_t _flow_update_col_shader = 0;
   int _flow_update_col_color_map = 0;
   int _flow_update_col_position_map = 0;
   int _flow_update_col_proj = 0;
   int _flow_update_col_stop = 0;

   // owns its own offscreen render target for capturing an infected player's color+depth each
   // frame - see FuseParticleSystem/PlayerDeathEffect for the same established pattern.
   std::unique_ptr<FrameBuffer> _deferred_buffer;
};
