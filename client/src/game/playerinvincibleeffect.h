#pragma once

#include "math/vector.h"
#include "render/texture.h"

#include <memory>
#include <unordered_map>

class Material;
class Matrix;
class FrameBuffer;
class PlayerInvincibleInstance;

class PlayerInvincibleEffect
{
public:
   PlayerInvincibleEffect();
   ~PlayerInvincibleEffect();

   void clear();
   void setRadius(float radius);

   void add(Material* player_material);
   void remove(Material* player_material);
   void animate(float dt);
   void render();

private:
   void blurPlayers(FrameBuffer* dst, const Matrix& proj);
   void setMaterialFade(PlayerInvincibleInstance* player, float fade);

   std::unordered_map<Material*, std::unique_ptr<PlayerInvincibleInstance>> _players;
   Texture _displacement_texture;
   float _radius;
   float _kernel[32];

   unsigned int _blur_h_shader;
   int _blur_h_texture;
   int _blur_h_texel_offset;
   int _blur_h_radius;
   int _blur_h_kernel;

   unsigned int _blur_v_shader;
   int _blur_v_texture;
   int _blur_v_texel_offset;
   int _blur_v_radius;
   int _blur_v_kernel;

   unsigned int _displace_shader;
   int _displace_texture1;
   int _displace_texture2;
   int _displace_texel_offset;
   int _displace_offset;
   int _displace_source_rect;
   int _displace_fade;
   int _display_center;

   FrameBuffer* _scratch_buffer;
};
