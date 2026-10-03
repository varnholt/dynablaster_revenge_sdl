#pragma once

#include "math/vector.h"
#include "render/texture.h"

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

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

   void add(Material& player_material);
   void remove(const Material& player_material);
   void animate(float dt);
   void render();

private:
   void blurPlayers(FrameBuffer& dst, const Matrix& proj);
   void setMaterialFade(PlayerInvincibleInstance& player, float fade);
   std::vector<std::unique_ptr<PlayerInvincibleInstance>>::iterator find(const Material& player_material);

   std::vector<std::unique_ptr<PlayerInvincibleInstance>> _players;
   Texture _displacement_texture;
   float _radius = 0.0f;
   float _kernel[32];

   uint32_t _blur_h_shader = 0;
   int _blur_h_texture = -1;
   int _blur_h_texel_offset = -1;
   int _blur_h_radius = -1;
   int _blur_h_kernel = -1;

   uint32_t _blur_v_shader = 0;
   int _blur_v_texture = -1;
   int _blur_v_texel_offset = -1;
   int _blur_v_radius = -1;
   int _blur_v_kernel = -1;

   uint32_t _displace_shader = 0;
   int _displace_texture1 = -1;
   int _displace_texture2 = -1;
   int _displace_texel_offset = -1;
   int _displace_offset = -1;
   int _displace_source_rect = -1;
   int _displace_fade = -1;
   int _display_center = -1;

   std::unique_ptr<FrameBuffer> _scratch_buffer;
};
