#pragma once

#include "filter.h"
#include "framework/framebuffer.h"
#include "fullscreenquad.h"

#include <array>
#include <cstdint>
#include <memory>

class BlurFilter : public Filter
{
public:
   BlurFilter(float radius = 1.0f);
   ~BlurFilter() override = default;

   bool init() override;

   void setRadius(float radius);
   void setAlpha(float alpha);

   void process(uint32_t texture, float u = 1.0f, float v = 1.0f) override;

private:
   void begin();
   void end();

   void downSamplePass(
      FrameBuffer& destination,
      int32_t destination_width,
      int32_t destination_height,
      uint32_t texture,
      int32_t source_width,
      int32_t source_height,
      float delta_u,
      float delta_v,
      float texel_u,
      float texel_v,
      float border
   );
   uint32_t downSample(uint32_t texture, int32_t width, int32_t height, int32_t pass);

   void gaussPass(
      FrameBuffer& destination,
      int32_t destination_width,
      int32_t destination_height,
      uint32_t texture,
      int32_t source_width,
      int32_t source_height,
      float delta_u,
      float delta_v,
      float texel_u,
      float texel_v
   );
   uint32_t gauss(float radius, uint32_t texture, int32_t width, int32_t height);

   void draw(int32_t destination_width, int32_t destination_height, uint32_t texture, int32_t source_width, int32_t source_height);

   uint32_t _downsample = 0;
   uint32_t _gauss = 0;

   // final upscale blit shader (GLES3 has no fixed-function textured quad)
   uint32_t _blit = 0;
   int32_t _param_blit_texture = -1;
   int32_t _param_blit_alpha = -1;

   int32_t _param_offset_x1 = -1;
   int32_t _param_offset_y1 = -1;
   int32_t _param_clamp_u1 = -1;
   int32_t _param_clamp_v1 = -1;
   int32_t _param_texture1 = -1;

   int32_t _param_offset_x2 = -1;
   int32_t _param_offset_y2 = -1;
   int32_t _param_texture2 = -1;
   int32_t _param_clamp_u2 = -1;
   int32_t _param_clamp_v2 = -1;
   int32_t _param_radius = -1;
   int32_t _param_kernel = -1;

   float _radius = 1.0f;
   std::array<std::unique_ptr<FrameBuffer>, 2> _temp;

   float _alpha = 1.0f;

   FullScreenQuad _quad;
};
