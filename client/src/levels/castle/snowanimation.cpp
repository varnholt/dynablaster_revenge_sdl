#include "snowanimation.h"

#include "constants.h"
#include "framework/framebuffer.h"
#include "framework/globaltime.h"
#include "gldevice.h"
#include "render/texturepool.h"
#include "tools/random.h"

#include <cmath>
#include <vector>

namespace
{
// flakes spawn slightly outside the unit square so the edges stay covered
constexpr float OFFSET_X = 0.2f;
constexpr float OFFSET_Y = 0.2f;

constexpr float LIMIT_DOWN = 0.0f;

// flakes stay hidden until the round's countdown is over
constexpr float VISIBLE_AFTER_MS = (SERVER_PREPARATION_TIME + 1) * 1000.0f;

constexpr int32_t FLOATS_PER_PARTICLE = 4;
}  // namespace

SnowAnimation::SnowAnimation()
{
   for (auto& particle : _particles)
   {
      initParticle(particle, frands(1.0f, 2.0f));
   }
}

SnowAnimation::~SnowAnimation()
{
   if (_vertex_buffer)
   {
      glDeleteBuffers(1, &_vertex_buffer);
   }
}

void SnowAnimation::initialize()
{
   _texture = TexturePool::Instance().getTexture("data/game/snowflake");
   _shader = activeDevice->loadShader("snow-vert.glsl", "snow-frag.glsl");
   _texture_param = activeDevice->getParameterIndex("texturemap");
   _size_factor_param = activeDevice->getParameterIndex("sizeFactor");

   glGenBuffers(1, &_vertex_buffer);
}

void SnowAnimation::initParticle(Particle& particle, float z)
{
   particle.position = Vector(frands(-OFFSET_X, 1.0f + OFFSET_X), frands(-OFFSET_Y, 1.0f + OFFSET_Y), z);

   // 30..50 px on a 1920 px wide screen
   particle.size = frands(30.0f, 50.0f);
   particle.speed = 0.8f + frand(0.2f);
}

float SnowAnimation::elapsedMs() const
{
   return GlobalTime::Instance()->getTime() * 1000.0f - _start_time;
}

void SnowAnimation::reset()
{
   _start_time = GlobalTime::Instance()->getTime() * 1000.0f;

   for (auto& particle : _particles)
   {
      particle.visible = false;
   }
}

void SnowAnimation::updateWind(float elapsed_ms)
{
   const float value = elapsed_ms * 0.0001f;
   _wind_direction = Vector(0.5f + std::sin(value) * 0.004f, std::cos(value) * 0.004f, -1.0f);
}

void SnowAnimation::animate(float dt)
{
   const float elapsed = elapsedMs();
   updateWind(elapsed);

   for (auto& particle : _particles)
   {
      if (particle.position.z < LIMIT_DOWN)
      {
         initParticle(particle, frands(1.0f, 2.0f));

         if (elapsed > VISIBLE_AFTER_MS)
         {
            particle.visible = true;
         }
      }

      particle.position += _wind_direction * particle.speed * (dt * 0.003f);
   }
}

void SnowAnimation::draw(const Matrix& transform)
{
   if (!_shader)
   {
      initialize();
   }

   std::vector<float> vertices;
   vertices.reserve(PARTICLE_COUNT * FLOATS_PER_PARTICLE);

   for (const auto& particle : _particles)
   {
      if (particle.visible)
      {
         vertices.insert(vertices.end(), {particle.position.x, particle.position.y, particle.position.z, particle.size});
      }
   }

   if (vertices.empty())
   {
      return;
   }

   // point sizes are authored for a 1920 px wide frame
   float size_factor = static_cast<float>(activeDevice->getWidth()) / 1920.0f;
   if (FrameBuffer* fb = FrameBuffer::Instance())
   {
      size_factor = fb->getSizeFactor(1920.0f);
   }

   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   activeDevice->setShader(_shader);
   activeDevice->setParameter(_size_factor_param, size_factor);

   // push() uploads the transform to the bound shader, so it has to follow setShader()
   activeDevice->push(transform);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _texture.getTexture());
   activeDevice->bindSampler(_texture_param, 0);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_DYNAMIC_DRAW);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, FLOATS_PER_PARTICLE, GL_FLOAT, GL_FALSE, sizeof(float) * FLOATS_PER_PARTICLE, nullptr);

   glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices.size() / FLOATS_PER_PARTICLE));

   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   activeDevice->pop();
   activeDevice->setShader(0);

   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_TRUE);
}
