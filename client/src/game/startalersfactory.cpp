#include "startalersfactory.h"

#include "framework/gldevice.h"
#include "image/image.h"
#include "math/matrix.h"
#include "tools/random.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace
{
constexpr int STAR_COUNT = 500;
constexpr float DURATION = 10.0f;

struct StarTalersVertex
{
   Vector position;
   Vector normal;
   float u;
   float v;
   Vector direction;
   float speed;
   float index;
};

Vector computeDirection(float width, float depth, float height)
{
   float alpha = 2.0f * std::numbers::pi_v<float> * frand(1.0f);
   float x = std::cos(alpha);
   float y = std::sin(alpha);
   float rand_x = frand(1.0f) * x;
   float rand_y = frand(1.0f) * y;

   Vector dir(rand_x, rand_y, 0.75f);
   dir.normalize();

   return Vector(width * dir.x, depth * dir.y, height * dir.z);
}

float computeSpeed(float min, float range)
{
   float speed = min + frand(range);

   if (frand(1.0f) > 0.8f)
   {
      speed *= 1.5f;
   }

   return speed;
}

void computeTextureOffset(float& u, float& v)
{
   if (frand(1.0f) < 0.75f)
   {
      u = 0.0f;
      v = 0.0f;
   }
   else
   {
      u = 0.5f;
      v = 0.0f;
   }
}
}  // namespace

StarTalersFactory::Burst::Burst(const Vector& field_position, const Vector& color) : _field_position(field_position), _color(color)
{
   int vertex_count = STAR_COUNT * 12;
   _index_count = STAR_COUNT * 18;

   _vertex_buffer = activeDevice().createVertexBuffer(vertex_count * static_cast<int>(sizeof(StarTalersVertex)));
   _index_buffer = activeDevice().createIndexBuffer(_index_count * static_cast<int>(sizeof(uint16_t)));

   const std::span<StarTalersVertex> vtx = activeDevice().lockVertexBuffer<StarTalersVertex>(_vertex_buffer);

   const float scale = 0.5f;
   int index = 0;

   for (int i = 0; i < vertex_count; i += 12)
   {
      Vector direction = computeDirection(1.3f, 1.3f, 2.75f);
      float speed = computeSpeed(0.9f, 0.2f);
      float u = 0.0f;
      float v = 0.0f;
      computeTextureOffset(u, v);

      const Vector positions[12] = {
         Vector(0.0f, 0.5f * scale, 0.5f * scale),
         Vector(0.0f, -0.5f * scale, 0.5f * scale),
         Vector(0.0f, -0.5f * scale, -0.5f * scale),
         Vector(0.0f, 0.5f * scale, -0.5f * scale),

         Vector(0.5f * scale, 0.0f, 0.5f * scale),
         Vector(-0.5f * scale, 0.0f, 0.5f * scale),
         Vector(-0.5f * scale, 0.0f, -0.5f * scale),
         Vector(0.5f * scale, 0.0f, -0.5f * scale),

         Vector(0.5f * scale, 0.5f * scale, 0.0f),
         Vector(-0.5f * scale, 0.5f * scale, 0.0f),
         Vector(-0.5f * scale, -0.5f * scale, 0.0f),
         Vector(0.5f * scale, -0.5f * scale, 0.0f),
      };

      const Vector normals[12] = {
         Vector(1, 0, 0),
         Vector(1, 0, 0),
         Vector(1, 0, 0),
         Vector(1, 0, 0),
         Vector(0, 1, 0),
         Vector(0, 1, 0),
         Vector(0, 1, 0),
         Vector(0, 1, 0),
         Vector(0, 0, 1),
         Vector(0, 0, 1),
         Vector(0, 0, 1),
         Vector(0, 0, 1),
      };

      const float us[12] = {
         u + 0.5f, u + 0.0f, u + 0.0f, u + 0.5f, u + 0.5f, u + 0.0f, u + 0.0f, u + 0.5f, u + 0.5f, u + 0.0f, u + 0.0f, u + 0.5f
      };
      const float vs[12] = {
         v + 0.5f, v + 0.5f, v + 0.0f, v + 0.0f, v + 0.5f, v + 0.5f, v + 0.0f, v + 0.0f, v + 0.5f, v + 0.5f, v + 0.0f, v + 0.0f
      };

      for (int k = 0; k < 12; k++)
      {
         vtx[i + k].position = positions[k];
         vtx[i + k].normal = normals[k];
         vtx[i + k].u = us[k];
         vtx[i + k].v = vs[k];
         vtx[i + k].direction = direction;
         vtx[i + k].speed = speed;
         vtx[i + k].index = static_cast<float>(index);
      }

      index++;
   }

   activeDevice().unlockVertexBuffer(_vertex_buffer);

   const std::span<uint16_t> idx = activeDevice().lockIndexBuffer<uint16_t>(_index_buffer);

   index = 0;
   for (int i = 0; i < _index_count; i += 18)
   {
      idx[i] = static_cast<uint16_t>(0 + index);
      idx[i + 1] = static_cast<uint16_t>(1 + index);
      idx[i + 2] = static_cast<uint16_t>(3 + index);

      idx[i + 3] = static_cast<uint16_t>(3 + index);
      idx[i + 4] = static_cast<uint16_t>(1 + index);
      idx[i + 5] = static_cast<uint16_t>(2 + index);

      idx[i + 6] = static_cast<uint16_t>(4 + index);
      idx[i + 7] = static_cast<uint16_t>(5 + index);
      idx[i + 8] = static_cast<uint16_t>(7 + index);

      idx[i + 9] = static_cast<uint16_t>(7 + index);
      idx[i + 10] = static_cast<uint16_t>(5 + index);
      idx[i + 11] = static_cast<uint16_t>(6 + index);

      idx[i + 12] = static_cast<uint16_t>(8 + index);
      idx[i + 13] = static_cast<uint16_t>(9 + index);
      idx[i + 14] = static_cast<uint16_t>(11 + index);

      idx[i + 15] = static_cast<uint16_t>(11 + index);
      idx[i + 16] = static_cast<uint16_t>(9 + index);
      idx[i + 17] = static_cast<uint16_t>(10 + index);

      index += 12;
   }

   activeDevice().unlockIndexBuffer(_index_buffer);
}

StarTalersFactory::Burst::~Burst()
{
   activeDevice().deleteBuffer(_vertex_buffer);
   activeDevice().deleteBuffer(_index_buffer);
}

bool StarTalersFactory::Burst::isElapsed() const
{
   return _time > DURATION;
}

void StarTalersFactory::Burst::update(float dt)
{
   _time += dt;
}

void StarTalersFactory::Burst::render(int field_param, int color_param, int time_param)
{
   activeDevice().setParameter(time_param, _time);
   activeDevice().setParameter(field_param, _field_position);
   activeDevice().setParameter(color_param, _color);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glEnableVertexAttribArray(3);
   glEnableVertexAttribArray(4);

   glVertexAttribPointer(
      0, 3, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), reinterpret_cast<const GLvoid*>(offsetof(StarTalersVertex, position))
   );
   glVertexAttribPointer(
      1, 3, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), reinterpret_cast<const GLvoid*>(offsetof(StarTalersVertex, normal))
   );
   glVertexAttribPointer(
      2, 2, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), reinterpret_cast<const GLvoid*>(offsetof(StarTalersVertex, u))
   );
   glVertexAttribPointer(
      3, 3, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), reinterpret_cast<const GLvoid*>(offsetof(StarTalersVertex, direction))
   );
   glVertexAttribPointer(
      4, 2, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), reinterpret_cast<const GLvoid*>(offsetof(StarTalersVertex, speed))
   );

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
   glDrawElements(GL_TRIANGLES, _index_count, GL_UNSIGNED_SHORT, 0);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(3);
   glDisableVertexAttribArray(4);
}

StarTalersFactory::StarTalersFactory()
{
   _color_bomb.set(0.047f, 0.49f, 0.012f);
   _color_flame.set(1.0f, 0.518f, 0.0f);
   _color_speedup.set(0.0f, 0.588f, 1.0f);
   _color_kick.set(0.635f, 0.0f, 1.0f);
   _color_default.set(1.0f, 1.0f, 1.0f);
}

StarTalersFactory::~StarTalersFactory()
{
   if (_texture_id)
   {
      activeDevice().deleteTexture(_texture_id);
   }
}

void StarTalersFactory::initialize()
{
   Image image;
   image.load("data/effects/startalers/startalers_particles");
   _texture_id = activeDevice().createTexture(image.getData(), image.getWidth(), image.getHeight());

   _shader = activeDevice().loadShader("startalers-vert.glsl", "startalers-frag.glsl");
   _field_param = activeDevice().getParameterIndex("field");
   _color_param = activeDevice().getParameterIndex("color");
   _time_param = activeDevice().getParameterIndex("time");
   _camera_param = activeDevice().getParameterIndex("camera");
   _texture_param = activeDevice().getParameterIndex("texturemap");
}

const Vector& StarTalersFactory::getColor(Constants::ExtraType extra) const
{
   switch (extra)
   {
      case Constants::ExtraBomb:
         return _color_bomb;
      case Constants::ExtraFlame:
         return _color_flame;
      case Constants::ExtraSpeedup:
         return _color_speedup;
      case Constants::ExtraKick:
         return _color_kick;
      default:
         return _color_default;
   }
}

void StarTalersFactory::add(float x, float y, Constants::ExtraType extra)
{
   Vector field_position(x + 0.5f, -(y + 0.5f), 0.0f);
   _bursts.push_back(std::make_unique<Burst>(field_position, getColor(extra)));
}

void StarTalersFactory::update(float dt)
{
   if (dt <= 0.0f)
   {
      return;
   }

   for (auto& burst : _bursts)
   {
      burst->update(dt);
   }

   std::erase_if(_bursts, [](const std::unique_ptr<Burst>& burst) { return burst->isElapsed(); });
}

void StarTalersFactory::render()
{
   if (_bursts.empty())
   {
      return;
   }

   glDisable(GL_CULL_FACE);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice().setShader(_shader);

   Vector camera = static_cast<GLDevice&>(activeDevice()).getProjectionMatrix().z();
   activeDevice().setParameter(_camera_param, camera);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _texture_id);
   activeDevice().bindSampler(_texture_param, 0);

   activeDevice().push(Matrix());

   for (auto& burst : _bursts)
   {
      burst->render(_field_param, _color_param, _time_param);
   }

   activeDevice().pop();

   activeDevice().setShader(0);

   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glDisable(GL_BLEND);
}
