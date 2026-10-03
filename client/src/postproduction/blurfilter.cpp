#include "blurfilter.h"
#include "gldevice.h"
#include "math/matrix.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
constexpr std::array<float, 9> filters = {0.0f, 4.9f, 16.8f, 50.726f, 136.882f, 345.38f, 848.614f, 1412.286f, 1612.833f};
constexpr std::array<float, 9> factors = {4.9f, 8.125f, 12.075f, 15.875f, 19.82f, 23.96875f, 28.0f, 18.100f, 18.0f};
constexpr int32_t kernel_size = 32;
}  // namespace

BlurFilter::BlurFilter(float radius) : Filter("blur"), _radius(radius)
{
}

bool BlurFilter::init()
{
   _downsample = activeDevice().loadShader("blur-vert.glsl", "blurprepass-simple-frag.glsl");
   _param_offset_x1 = activeDevice().getParameterIndex("texelOffsetX");
   _param_offset_y1 = activeDevice().getParameterIndex("texelOffsetY");
   _param_clamp_u1 = activeDevice().getParameterIndex("clampU");
   _param_clamp_v1 = activeDevice().getParameterIndex("clampV");
   _param_texture1 = activeDevice().getParameterIndex("texture0");

   _gauss = activeDevice().loadShader("blur-vert.glsl", "blur-frag.glsl");
   _param_offset_x2 = activeDevice().getParameterIndex("texelOffsetX");
   _param_offset_y2 = activeDevice().getParameterIndex("texelOffsetY");
   _param_texture2 = activeDevice().getParameterIndex("texture0");
   _param_clamp_u2 = activeDevice().getParameterIndex("clampU");
   _param_clamp_v2 = activeDevice().getParameterIndex("clampV");
   _param_radius = activeDevice().getParameterIndex("radius");
   _param_kernel = activeDevice().getParameterIndex("kernel");

   _blit = activeDevice().loadShader("blur-blit-vert.glsl", "blur-blit-frag.glsl");
   _param_blit_texture = activeDevice().getParameterIndex("texture0");
   _param_blit_alpha = activeDevice().getParameterIndex("alpha");

   return true;
}

void BlurFilter::setRadius(float radius)
{
   _radius = radius;
}

void BlurFilter::setAlpha(float alpha)
{
   _alpha = alpha;
}

uint32_t BlurFilter::downSample(uint32_t texture, int32_t width, int32_t height, int32_t pass)
{
   const float delta_u = 1.0f / _temp[0]->width();
   const float delta_v = 1.0f / _temp[0]->height();

   activeDevice().setShader(_downsample);
   activeDevice().bindSampler(_param_texture1, 0);
   activeDevice().setParameter(_param_clamp_u1, delta_u * (width - 0.5f));
   activeDevice().setParameter(_param_clamp_v1, delta_v * (height - 0.5f));

   const float border = 1.0f;
   downSamplePass(*_temp[0], width >> 1, height, texture, width, height, delta_u, delta_v, delta_u * pass, 0.0f, border);
   downSamplePass(
      *_temp[1], width >> 1, height >> 1, _temp[0]->texture(), width >> 1, height, delta_u, delta_v, 0.0f, delta_v * pass, border
   );

   return _temp[1]->texture();
}

// horizontal or vertical pass of downsampling
void BlurFilter::downSamplePass(
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
)
{
   glBindTexture(GL_TEXTURE_2D, texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   activeDevice().setParameter(_param_offset_x1, texel_u);
   activeDevice().setParameter(_param_offset_y1, texel_v);

   destination.bind();

   const float w = destination_width + border;
   const float h = destination_height + border;

   // fix texture coordinate to match additional border radius
   const float tu1 = 0.0f;
   const float tv1 = 0.0f;
   const float tu2 = (source_width + border * source_width / destination_width) * delta_u;
   const float tv2 = (source_height + border * source_height / destination_height) * delta_v;

   static_cast<GLDevice&>(activeDevice())
      .setProjectionMatrix(
         Matrix::ortho(0.0f, static_cast<float>(destination.width()), 0.0f, static_cast<float>(destination.height()), -1.0f, 1.0f)
      );
   activeDevice().push(Matrix());
   _quad.drawRect(0.0f, 0.0f, w, h, tu1, tv1, tu2, tv2);

   destination.unbind();
}

uint32_t BlurFilter::gauss(float radius, uint32_t texture, int32_t width, int32_t height)
{
   const float delta_u = 1.0f / _temp[0]->width();
   const float delta_v = 1.0f / _temp[0]->height();

   const int32_t size = std::min(static_cast<int32_t>(std::ceil(radius)), kernel_size - 1);

   std::array<float, kernel_size> kernel{};
   const double scale = -4.0 / (radius * radius);
   float sum = 0.0f;
   kernel[0] = 1.0f;
   for (int32_t i = 1; i <= size; i++)
   {
      const auto f = static_cast<float>(std::pow(std::numbers::e, i * i * scale));
      sum += f;
      kernel[i] = f;
   }

   const float t = 1.0f / (sum * 2.0f + 1.0f);
   for (int32_t i = 0; i <= size; i++)
   {
      kernel[i] *= t;
   }

   activeDevice().setShader(_gauss);
   activeDevice().setParameter(_param_kernel, kernel);
   activeDevice().setParameter(_param_radius, size + 0.5f);
   activeDevice().bindSampler(_param_texture2, 0);

   activeDevice().setParameter(_param_clamp_u2, delta_u * (width - 0.5f));
   activeDevice().setParameter(_param_clamp_v2, delta_v * (height - 0.5f));

   gaussPass(*_temp[0], width, height, texture, width, height, delta_u, delta_v, delta_u, 0.0f);
   gaussPass(*_temp[1], width, height, _temp[0]->texture(), width, height, delta_u, delta_v, 0.0f, delta_v);

   return _temp[1]->texture();
}

void BlurFilter::gaussPass(
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
)
{
   glBindTexture(GL_TEXTURE_2D, texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   activeDevice().setParameter(_param_offset_x2, texel_u);
   activeDevice().setParameter(_param_offset_y2, texel_v);

   destination.bind();

   const auto w = static_cast<float>(destination_width);
   const auto h = static_cast<float>(destination_height);

   const float tu1 = 0.0f;
   const float tv1 = 0.0f;
   const float tu2 = source_width * delta_u;
   const float tv2 = source_height * delta_v;

   static_cast<GLDevice&>(activeDevice())
      .setProjectionMatrix(
         Matrix::ortho(0.0f, static_cast<float>(destination.width()), 0.0f, static_cast<float>(destination.height()), -1.0f, 1.0f)
      );
   activeDevice().push(Matrix());
   _quad.drawRect(0.0f, 0.0f, w, h, tu1, tv1, tu2, tv2);

   destination.unbind();
}

void BlurFilter::draw(int32_t destination_width, int32_t destination_height, uint32_t texture, int32_t source_width, int32_t source_height)
{
   activeDevice().setShader(_blit);
   activeDevice().setParameter(_param_blit_alpha, _alpha);
   activeDevice().bindSampler(_param_blit_texture, 0);

   glBindTexture(GL_TEXTURE_2D, texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   const int32_t w = destination_width;
   const int32_t h = destination_height;

   const float tu1 = 0.0f;
   const float tv1 = 0.0f;
   const float tu2 = static_cast<float>(source_width) / w;
   const float tv2 = static_cast<float>(source_height) / h;

   const auto frame_buffer = FrameBuffer::Instance();
   const int32_t frame_buffer_width = frame_buffer ? frame_buffer->get().width() : w;
   const int32_t frame_buffer_height = frame_buffer ? frame_buffer->get().height() : h;

   static_cast<GLDevice&>(activeDevice())
      .setProjectionMatrix(
         Matrix::ortho(0.0f, static_cast<float>(frame_buffer_width), 0.0f, static_cast<float>(frame_buffer_height), -1.0f, 1.0f)
      );
   activeDevice().push(Matrix());
   _quad.drawRect(0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h), tu1, tv1, tu2, tv2);

   activeDevice().setShader(0);
}

void BlurFilter::begin()
{
   static_cast<GLDevice&>(activeDevice()).pushProjection();

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
}

void BlurFilter::end()
{
   static_cast<GLDevice&>(activeDevice()).popProjection();

   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void BlurFilter::process(uint32_t texture, float, float)
{
   begin();

   const auto frame_buffer = FrameBuffer::Instance();

   FrameBuffer::push();

   // create buffers
   for (auto& buffer : _temp)
   {
      if (!buffer || buffer->resolutionChanged(frame_buffer->get().width(), frame_buffer->get().height()))
      {
         buffer.reset();
         buffer = std::make_unique<FrameBuffer>(frame_buffer->get().width(), frame_buffer->get().height(), 1, FrameBuffer::NoDepthBuffer);
      }
   }

   int32_t width = frame_buffer->get().width();
   int32_t height = frame_buffer->get().height();

   int32_t prepasses = 0;
   float gauss_radius = _radius;
   while (gauss_radius > filters[prepasses + 1] && prepasses < 7)
   {
      prepasses++;
   }
   gauss_radius = (gauss_radius - filters[prepasses]) * factors[prepasses] / (filters[prepasses + 1] - filters[prepasses]);

   // downsampling passes
   for (int32_t pass = 1; pass <= prepasses; pass++)
   {
      texture = downSample(texture, width, height, pass);
      width >>= 1;
      height >>= 1;
   }

   // gauss pass with remaining radius
   texture = gauss(gauss_radius, texture, width, height);

   FrameBuffer::pop();

   draw(frame_buffer->get().width(), frame_buffer->get().height(), texture, width, height);

   end();
}
