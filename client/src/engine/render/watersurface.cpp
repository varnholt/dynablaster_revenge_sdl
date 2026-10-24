#include "watersurface.h"

#include "engine/gltf/gltfmodel.h"
#include "framework/framebuffer.h"
#include "framework/globaltime.h"
#include "gldevice.h"
#include "math/vector2.h"

#include <algorithm>

namespace
{
// what the water reflects where nothing stands, a hazy sky
constexpr float SKY_R = 0.48f;
constexpr float SKY_G = 0.68f;
constexpr float SKY_B = 0.92f;
}  // namespace

WaterSurface::WaterSurface()
{
   _shader = activeDevice().loadShader("water-vert.glsl", "water-frag.glsl");
   _data_param = activeDevice().getParameterIndex("u_data");
   _reflection_param = activeDevice().getParameterIndex("u_reflection");
   _refraction_param = activeDevice().getParameterIndex("u_refraction");
   _time_param = activeDevice().getParameterIndex("u_time");
   _eye_param = activeDevice().getParameterIndex("u_eye");
   _deep_param = activeDevice().getParameterIndex("u_deep");
   _origin_param = activeDevice().getParameterIndex("u_origin");
   _size_param = activeDevice().getParameterIndex("u_size");
   _haze_param = activeDevice().getParameterIndex("u_haze");
   _haze_box_param = activeDevice().getParameterIndex("u_haze_box");
   _haze_range_param = activeDevice().getParameterIndex("u_haze_range");

   glGenTextures(1, &_refraction);
}

WaterSurface::~WaterSurface()
{
   glDeleteTextures(1, &_refraction);
}

void WaterSurface::renderReflection(float level, const std::function<void(const Matrix&)>& draw)
{
   const RenderDevice::ViewPort viewport = activeDevice().getViewPort();
   const int32_t width = std::max(1, viewport.width / 2);
   const int32_t height = std::max(1, viewport.height / 2);

   if (!_reflection)
   {
      _reflection = std::make_unique<FrameBuffer>(width, height);
   }
   else if (_reflection->resolutionChanged(width, height))
   {
      _reflection->setResolution(width, height);
   }

   FrameBuffer::push(*_reflection);
   static_cast<GLDevice&>(activeDevice()).clear(SKY_R, SKY_G, SKY_B, 1.0f);

   // mirrored the triangles turn around
   glFrontFace(GL_CW);
   GltfModel::setClipHeight(level + 0.01f);

   draw(Matrix::scale(1.0f, 1.0f, -1.0f) * Matrix::position(0.0f, 0.0f, 2.0f * level));

   GltfModel::setClipHeight(1e9f);
   glFrontFace(GL_CCW);

   FrameBuffer::pop();
   activeDevice().setViewPort(viewport.x, viewport.y, viewport.width, viewport.height);
}

void WaterSurface::captureRefraction()
{
   const auto [x, y, width, height] = activeDevice().getViewPort();

   glBindTexture(GL_TEXTURE_2D, _refraction);
   FrameBuffer::copyTexImage(x, y, width, height);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glBindTexture(GL_TEXTURE_2D, 0);
}

void WaterSurface::render(GltfModel& model, const Vector& eye)
{
   if (!_reflection)
   {
      return;
   }

   const RenderDevice::ViewPort viewport = activeDevice().getViewPort();

   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_data_param, 0);
   activeDevice().bindSampler(_reflection_param, 1);
   activeDevice().bindSampler(_refraction_param, 2);
   activeDevice().setParameter(_time_param, GlobalTime::Instance().getTime());
   activeDevice().setParameter(_eye_param, eye);
   activeDevice().setParameter(_deep_param, model.getWaterColor());
   activeDevice().setParameter(_origin_param, Vector2(static_cast<float>(viewport.x), static_cast<float>(viewport.y)));
   activeDevice().setParameter(_size_param, Vector2(static_cast<float>(viewport.width), static_cast<float>(viewport.height)));

   // the same air as the scenery around it
   const GltfModel::Atmosphere& air = GltfModel::getCurrentAtmosphere();
   activeDevice().setParameter(_haze_param, Vector4(air._haze.x, air._haze.y, air._haze.z, air._strength));
   activeDevice().setParameter(_haze_box_param, air._box);
   activeDevice().setParameter(_haze_range_param, Vector2(air._near, std::max(air._far, air._near + 0.001f)));

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _reflection->texture());
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, _refraction);
   glActiveTexture(GL_TEXTURE0);

   for (GLuint attribute = 0; attribute < 3; attribute++)
   {
      glEnableVertexAttribArray(attribute);
   }

   model.drawWater(Matrix());

   for (GLuint attribute = 0; attribute < 3; attribute++)
   {
      glDisableVertexAttribArray(attribute);
   }

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);

   activeDevice().pop();
   activeDevice().setShader(0);
}
