#include "fuseparticlesystem.h"

#include "framework/gldevice.h"
#include "image/image.h"
#include "math/matrix.h"
#include "tools/random.h"

namespace
{
constexpr int PARTICLE_COUNT = 100;

// offset from a bomb mesh's origin to where its fuse actually sits
constexpr float BOMB_OFFSET_X = 0.36f;
constexpr float BOMB_OFFSET_Y = 0.15f;
constexpr float BOMB_OFFSET_Z = 0.48f;

// spark size in screen pixels at unit view-space depth
constexpr float PARTICLE_PIXEL_SIZE = 48.0f;
}  // namespace

FuseParticleSystem::FuseParticleSystem()
{
   Image image;
   image.load("data/logo/pointsprite");
   _particle_texture_id = activeDevice().createTexture(image.getData(), image.getWidth(), image.getHeight());

   _shader = activeDevice().loadShader("fuseparticles-vert.glsl", "fuseparticles-frag.glsl");
   _texture = activeDevice().getParameterIndex("texturemap");
   _point_size = activeDevice().getParameterIndex("particleSize");
   _projection = activeDevice().getParameterIndex("u_projection");
}

FuseParticleSystem::~FuseParticleSystem()
{
   if (_vertex_buffer)
   {
      glDeleteBuffers(1, &_vertex_buffer);
   }
   if (_particle_texture_id)
   {
      activeDevice().deleteTexture(_particle_texture_id);
   }
}

const Vector& FuseParticleSystem::getBombOffset()
{
   static const Vector offset(BOMB_OFFSET_X, BOMB_OFFSET_Y, BOMB_OFFSET_Z);
   return offset;
}

void FuseParticleSystem::resetParticle(Particle& particle, const Vector& origin)
{
   particle.origin = origin;
   particle.origin.x += frand(0.099f);
   particle.origin.y += frand(0.099f);
   particle.origin.z += frand(0.099f);

   particle.position = particle.origin;
   particle.point_size = 1.0f;
   particle.scalar = 0.0f;
   particle.direction = Vector(frands(-0.225f, 0.375f), frands(-0.225f, 0.375f), frands(-0.225f, 0.225f));
   particle.elapsed = 0.0f;
   particle.random_start_time = frand(100.0f);
   particle.started = false;
}

void FuseParticleSystem::addEmitter(MapItem* item, const Vector& origin)
{
   Emitter emitter;
   emitter.origin = origin;
   emitter.next_origin = origin;
   emitter.removing = false;
   emitter.particles.resize(PARTICLE_COUNT);

   for (auto& particle : emitter.particles)
   {
      resetParticle(particle, origin);
   }

   _emitters[item] = std::move(emitter);
}

void FuseParticleSystem::setEmitterPosition(MapItem* item, const Vector& origin)
{
   auto it = _emitters.find(item);
   if (it == _emitters.end())
   {
      return;
   }

   Emitter& emitter = it->second;
   if (emitter.origin != origin)
   {
      emitter.origin = origin;
      emitter.next_origin = origin;
   }
}

void FuseParticleSystem::removeEmitter(MapItem* item)
{
   auto it = _emitters.find(item);
   if (it != _emitters.end())
   {
      it->second.removing = true;
   }
}

void FuseParticleSystem::animate(float dt)
{
   for (auto it = _emitters.begin(); it != _emitters.end();)
   {
      Emitter& emitter = it->second;

      for (auto pit = emitter.particles.begin(); pit != emitter.particles.end();)
      {
         Particle& particle = *pit;

         particle.elapsed += dt;

         // bomb kicked far away - burn down instead of dragging the trail across the map
         float dist = (particle.origin - emitter.next_origin).length();
         if (dist > 1.0f)
         {
            particle.random_start_time = 0.0f;
         }

         bool remove = false;

         if (particle.elapsed > particle.random_start_time || emitter.removing)
         {
            particle.started = true;

            particle.direction.y -= dt * 0.02f;
            particle.scalar += dt * 0.02f;
            particle.point_size -= dt * 0.025f;

            particle.position = particle.origin + particle.direction * particle.scalar;

            if (particle.point_size < 0.01f)
            {
               if (emitter.removing)
               {
                  remove = true;
               }
               else
               {
                  resetParticle(particle, emitter.next_origin);
               }
            }
         }

         if (remove)
         {
            pit = emitter.particles.erase(pit);
         }
         else
         {
            ++pit;
         }
      }

      if (emitter.removing && emitter.particles.empty())
      {
         it = _emitters.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

void FuseParticleSystem::render()
{
   _upload_buffer.clear();

   for (const auto& [item, emitter] : _emitters)
   {
      for (const auto& particle : emitter.particles)
      {
         if (!particle.started)
         {
            continue;
         }

         _upload_buffer.push_back(particle.position.x);
         _upload_buffer.push_back(particle.position.y);
         _upload_buffer.push_back(particle.position.z);
         _upload_buffer.push_back(particle.point_size);
      }
   }

   if (_upload_buffer.empty())
   {
      return;
   }

   if (_vertex_buffer == 0)
   {
      glGenBuffers(1, &_vertex_buffer);
   }

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(float) * _upload_buffer.size()), _upload_buffer.data(), GL_DYNAMIC_DRAW);

   activeDevice().setShader(_shader);
   activeDevice().setParameter(_projection, static_cast<GLDevice&>(activeDevice()).getProjectionMatrix());
   activeDevice().setParameter(_point_size, PARTICLE_PIXEL_SIZE);
   activeDevice().push(Matrix());

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glDepthMask(GL_FALSE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _particle_texture_id);
   activeDevice().bindSampler(_texture, 0);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(float) * 4), (GLvoid*)0);

   glDrawArrays(GL_POINTS, 0, static_cast<int>(_upload_buffer.size() / 4));

   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);

   activeDevice().pop();
   activeDevice().setShader(0);
}
