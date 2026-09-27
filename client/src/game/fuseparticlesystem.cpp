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
 : mVertexBuffer(0),
   mParticleTextureId(0),
   mShader(0),
   mTexture(0),
   mPointSize(0),
   mProjection(0)
{
   Image image;
   image.load("data/logo/pointsprite");
   mParticleTextureId = activeDevice->createTexture(image.getData(), image.getWidth(), image.getHeight());

   mShader = activeDevice->loadShader("fuseparticles-vert.glsl", "fuseparticles-frag.glsl");
   mTexture = activeDevice->getParameterIndex("texturemap");
   mPointSize = activeDevice->getParameterIndex("particleSize");
   mProjection = activeDevice->getParameterIndex("u_projection");
}

FuseParticleSystem::~FuseParticleSystem()
{
   if (mVertexBuffer)
   {
      glDeleteBuffers(1, &mVertexBuffer);
   }
   if (mParticleTextureId)
   {
      activeDevice->deleteTexture(mParticleTextureId);
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
   particle.pointSize = 1.0f;
   particle.scalar = 0.0f;
   particle.direction = Vector(frands(-0.225f, 0.375f), frands(-0.225f, 0.375f), frands(-0.225f, 0.225f));
   particle.elapsed = 0.0f;
   particle.randomStartTime = frand(100.0f);
   particle.started = false;
}

void FuseParticleSystem::addEmitter(MapItem* item, const Vector& origin)
{
   Emitter emitter;
   emitter.origin = origin;
   emitter.nextOrigin = origin;
   emitter.removing = false;
   emitter.particles.resize(PARTICLE_COUNT);

   for (auto& particle : emitter.particles)
   {
      resetParticle(particle, origin);
   }

   mEmitters[item] = std::move(emitter);
}

void FuseParticleSystem::setEmitterPosition(MapItem* item, const Vector& origin)
{
   auto it = mEmitters.find(item);
   if (it == mEmitters.end())
   {
      return;
   }

   Emitter& emitter = it->second;
   if (emitter.origin != origin)
   {
      emitter.origin = origin;
      emitter.nextOrigin = origin;
   }
}

void FuseParticleSystem::removeEmitter(MapItem* item)
{
   auto it = mEmitters.find(item);
   if (it != mEmitters.end())
   {
      it->second.removing = true;
   }
}

void FuseParticleSystem::animate(float dt)
{
   for (auto it = mEmitters.begin(); it != mEmitters.end();)
   {
      Emitter& emitter = it->second;

      for (auto pit = emitter.particles.begin(); pit != emitter.particles.end();)
      {
         Particle& particle = *pit;

         particle.elapsed += dt;

         // bomb kicked far away - burn down instead of dragging the trail across the map
         float dist = (particle.origin - emitter.nextOrigin).length();
         if (dist > 1.0f)
         {
            particle.randomStartTime = 0.0f;
         }

         bool remove = false;

         if (particle.elapsed > particle.randomStartTime || emitter.removing)
         {
            particle.started = true;

            particle.direction.y -= dt * 0.02f;
            particle.scalar += dt * 0.02f;
            particle.pointSize -= dt * 0.025f;

            particle.position = particle.origin + particle.direction * particle.scalar;

            if (particle.pointSize < 0.01f)
            {
               if (emitter.removing)
               {
                  remove = true;
               }
               else
               {
                  resetParticle(particle, emitter.nextOrigin);
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
         it = mEmitters.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

void FuseParticleSystem::render()
{
   mUploadBuffer.clear();

   for (const auto& [item, emitter] : mEmitters)
   {
      for (const auto& particle : emitter.particles)
      {
         if (!particle.started)
         {
            continue;
         }

         mUploadBuffer.push_back(particle.position.x);
         mUploadBuffer.push_back(particle.position.y);
         mUploadBuffer.push_back(particle.position.z);
         mUploadBuffer.push_back(particle.pointSize);
      }
   }

   if (mUploadBuffer.empty())
   {
      return;
   }

   if (mVertexBuffer == 0)
   {
      glGenBuffers(1, &mVertexBuffer);
   }

   glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);
   glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(float) * mUploadBuffer.size()), mUploadBuffer.data(), GL_DYNAMIC_DRAW);

   activeDevice->setShader(mShader);
   activeDevice->setParameter(mProjection, static_cast<GLDevice*>(activeDevice)->getProjectionMatrix());
   activeDevice->setParameter(mPointSize, PARTICLE_PIXEL_SIZE);
   activeDevice->push(Matrix());

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);
   glDepthMask(GL_FALSE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, mParticleTextureId);
   activeDevice->bindSampler(mTexture, 0);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(sizeof(float) * 4), (GLvoid*)0);

   glDrawArrays(GL_POINTS, 0, static_cast<int>(mUploadBuffer.size() / 4));

   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glDepthMask(GL_TRUE);
   glDisable(GL_BLEND);

   activeDevice->pop();
   activeDevice->setShader(0);
}
