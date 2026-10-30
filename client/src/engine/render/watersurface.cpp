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

WaterSurface::WaterSurface(bool lava) : _lava(lava)
{
   _shader = activeDevice().loadShader("water-vert.glsl", lava ? "lava-frag.glsl" : "water-frag.glsl");
   _data_param = activeDevice().getParameterIndex("u_data");
   _reflection_param = activeDevice().getParameterIndex("u_reflection");
   _refraction_param = activeDevice().getParameterIndex("u_refraction");
   _time_param = activeDevice().getParameterIndex("u_time");
   _eye_param = activeDevice().getParameterIndex("u_eye");
   _deep_param = activeDevice().getParameterIndex("u_deep");
   _origin_param = activeDevice().getParameterIndex("u_origin");
   _size_param = activeDevice().getParameterIndex("u_size");
   _haze_param = activeDevice().getParameterIndex("u_haze");
   _flow_param = activeDevice().getParameterIndex("u_flow");
   _glitter_param = activeDevice().getParameterIndex("u_glitter");
   _haze_box_param = activeDevice().getParameterIndex("u_haze_box");
   _haze_range_param = activeDevice().getParameterIndex("u_haze_range");
   _waders_param = activeDevice().getParameterIndex("u_waders");
   _wader_count_param = activeDevice().getParameterIndex("u_wader_count");
   _sun_direction_param = activeDevice().getParameterIndex("u_sun_direction");
   _sun_param = activeDevice().getParameterIndex("u_sun");

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

bool WaterSurface::isLava() const
{
   return _lava;
}

void WaterSurface::setWaders(std::vector<Wader> waders)
{
   _waders = std::move(waders);
   _waders.resize(std::min(_waders.size(), MAX_WADERS));
}

void WaterSurface::render(GltfModel& model, const Vector& eye, const Matrix& transform)
{
   if (!_reflection && !_lava)
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
   // the deep colour is the daytime one, the night darkens it
   activeDevice().setParameter(_flow_param, model.getWaterFlow());
   activeDevice().setParameter(_glitter_param, model.getWaterGlitter());
   activeDevice().setParameter(_deep_param, model.getWaterColor() * (0.15f + 0.85f * GltfModel::getDaylight()));
   activeDevice().setParameter(_origin_param, Vector2(static_cast<float>(viewport.x), static_cast<float>(viewport.y)));
   activeDevice().setParameter(_size_param, Vector2(static_cast<float>(viewport.width), static_cast<float>(viewport.height)));

   // the same air as the scenery around it
   const GltfModel::Atmosphere& air = GltfModel::getCurrentAtmosphere();
   activeDevice().setParameter(_haze_param, Vector4(air._haze.x, air._haze.y, air._haze.z, air._strength));
   activeDevice().setParameter(_haze_box_param, air._box);
   activeDevice().setParameter(_haze_range_param, Vector2(air._near, std::max(air._far, air._near + 0.001f)));
   activeDevice().setParameter(_sun_direction_param, air._sun_direction);
   activeDevice().setParameter(_sun_param, air._sun);

   std::vector<float> waders;
   waders.reserve(_waders.size() * 4);
   for (const Wader& wader : _waders)
   {
      waders.insert(waders.end(), {wader._position.x, wader._position.y, wader._velocity.x, wader._velocity.y});
   }

   if (!waders.empty())
   {
      glUniform4fv(_waders_param, static_cast<GLsizei>(_waders.size()), waders.data());
   }

   activeDevice().setParameter(_wader_count_param, static_cast<float>(_waders.size()));

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _reflection ? _reflection->texture() : 0);
   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, _refraction);
   glActiveTexture(GL_TEXTURE0);

   for (GLuint attribute = 0; attribute < 3; attribute++)
   {
      glEnableVertexAttribArray(attribute);
   }

   model.drawWater(transform);

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
