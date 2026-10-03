#include "lensflarefactory.h"

#include "lensflare.h"

#include "gldevice.h"
#include "math/matrix.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "settings.h"

namespace
{
// the flare is laid out in a fixed 16:9 ortho space
constexpr float ASPECT_Y = 9.0f / 16.0f;
}  // namespace

LensFlareFactory::LensFlareFactory()
{
   _shader = activeDevice().loadShader("lensflare-vert.glsl", "lensflare-frag.glsl");
   _time_param = activeDevice().getParameterIndex("time");
   _sun_param = activeDevice().getParameterIndex("sun");
   _length_param = activeDevice().getParameterIndex("normalizedlength");
   _texture_param = activeDevice().getParameterIndex("texturemap");

   Settings settings("data/effects/lensflare/flares/flares.ini");

   for (const auto& group : settings.value("main/groups").toStringList())
   {
      LensFlare::Config config;
      config.texture = settings.value(group + "/texture").toString();
      config.ghosts = settings.value(group + "/ghosts").toString();
      config.sun_3d = Vector(
         settings.value(group + "/sun_x").toFloat(), settings.value(group + "/sun_y").toFloat(), settings.value(group + "/sun_z").toFloat()
      );
      config.max_length = settings.value(group + "/maxlength").toFloat();
      config.lower_limit = settings.value(group + "/lowerlimit").toFloat();
      config.angle = settings.value(group + "/angle").toFloat();
      config.invert_offset = settings.value(group + "/invert").toFloat(&config.inverted);
      config.sun_is_2d = settings.value(group + "/2d", false).toBool();

      _lens_flares[group] = std::make_unique<LensFlare>(config);
   }
}

LensFlareFactory::~LensFlareFactory() = default;

bool LensFlareFactory::activate(const std::string& key)
{
   _active = _lens_flares.find(key);
   return _active != _lens_flares.end();
}

void LensFlareFactory::draw()
{
   if (_active == _lens_flares.end())
   {
      return;
   }

   auto& device = static_cast<GLDevice&>(activeDevice());

   // sun position on screen, from the scene camera (the view lives in the projection matrix)
   const Matrix projection = device.getProjectionMatrix();
   const auto& config = _active->second->getConfig();
   const Vector4 projected = projection * Vector4(config.sun_3d.x, config.sun_3d.y, config.sun_3d.z);

   Vector2 sun_2d(projected.x / projected.w, (projected.y / projected.w) * ASPECT_Y);
   if (config.sun_is_2d)
   {
      sun_2d = Vector2(config.sun_3d.x + projection.xw * 0.1f + 0.5f, config.sun_3d.y * ASPECT_Y);
   }

   device.pushProjection();
   device.setProjectionMatrix(Matrix::ortho(-1.0f, 1.0f, -ASPECT_Y, ASPECT_Y, -10.0f, 10.0f));

   device.setCulling(false);
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   device.setShader(_shader);
   device.push(Matrix());
   device.bindSampler(_texture_param, 0);

   _active->second->draw(sun_2d, _time_param, _sun_param, _length_param);

   device.pop();
   device.setShader(0);

   device.setCulling(true);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   device.popProjection();
}
