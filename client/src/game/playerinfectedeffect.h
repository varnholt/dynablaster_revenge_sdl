#pragma once

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

   float _delta;

   unsigned int _particle_texture_id;
   unsigned int _flow_field_texture_id;

   unsigned int _points_shader;
   int _points_texture;
   int _points_size;
   int _points_projection;

   unsigned int _flow_update_pos_shader;
   int _flow_depth_texture;
   int _flow_vertex_pos_texture;
   int _flow_vertex_param_texture;
   int _flow_field_texture;
   int _flow_center;
   int _flow_field_scale;
   int _flow_time_delta;
   int _flow_src_rect;
   int _flow_inv_proj;
   int _flow_stop;

   unsigned int _flow_init_pos_shader;
   int _flow_init_pos_depth;
   int _flow_init_pos_inv_proj;
   int _flow_init_src_rect;

   unsigned int _flow_init_param_shader;
   int _flow_init_param_depth;
   int _flow_init_param_inv_proj;
   int _flow_init_param_center;

   unsigned int _flow_update_col_shader;
   int _flow_update_col_color_map;
   int _flow_update_col_position_map;
   int _flow_update_col_proj;
   int _flow_update_col_stop;

   // owns its own offscreen render target for capturing an infected player's color+depth each
   // frame - see FuseParticleSystem/PlayerDeathEffect for the same established pattern.
   FrameBuffer* _deferred_buffer;
};
