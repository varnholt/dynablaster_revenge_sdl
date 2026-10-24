#include "nightsky.h"

#include "framework/globaltime.h"
#include "gldevice.h"
#include "render/texturepool.h"

#include <array>

namespace
{
// one triangle covering the screen
constexpr std::array<float, 6> TRIANGLE{-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
}  // namespace

NightSky::NightSky(const std::string& sky)
{
   _sky = TexturePool::Instance().getTexture(sky, TexturePool::Linear);
   _shader = activeDevice().loadShader("nightsky-vert.glsl", "nightsky-frag.glsl");
   _inverse_param = activeDevice().getParameterIndex("u_inverse");
   _sky_param = activeDevice().getParameterIndex("u_sky");
   _time_param = activeDevice().getParameterIndex("u_time");
   _brightness_param = activeDevice().getParameterIndex("u_brightness");
   _moon_param = activeDevice().getParameterIndex("u_moon");
   _moon_size_param = activeDevice().getParameterIndex("u_moon_size");

   glGenBuffers(1, &_vertex_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(TRIANGLE), TRIANGLE.data(), GL_STATIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

NightSky::~NightSky()
{
   glDeleteBuffers(1, &_vertex_buffer);
}

void NightSky::render()
{
   GLDevice& device = static_cast<GLDevice&>(activeDevice());

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   device.setShader(_shader);
   device.setParameter(_inverse_param, device.getProjectionMatrix().invert4x4());
   device.setParameter(_time_param, GlobalTime::Instance().getTime());
   device.setParameter(_brightness_param, _brightness);
   device.setParameter(_moon_param, _moon);
   device.setParameter(_moon_size_param, _moon_size);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _sky.getTexture());
   device.bindSampler(_sky_param, 0);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
   glDrawArrays(GL_TRIANGLES, 0, 3);
   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   device.setShader(0);
   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
}

void NightSky::setBrightness(float brightness)
{
   _brightness = brightness;
}

void NightSky::setMoon(const Vector& direction, float size)
{
   _moon = direction;
   _moon_size = size;
}
