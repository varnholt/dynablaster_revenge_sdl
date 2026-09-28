#include "playerinfectedeffect.h"
#include "infectedflowfieldanimation.h"
#include "framework/framebuffer.h"
#include "framework/gldevice.h"
#include "materials/material.h"
#include "nodes/node.h"
#include "nodes/mesh.h"
#include "render/geometry.h"
#include "math/vector.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "math/matrix.h"
#include "image/image.h"

PlayerInfectedEffect::PlayerInfectedEffect()
   : mDelta(0.0f),
     mParticleTextureId(0),
     mFlowFieldTextureId(0),
     mPointsShader(0),
     mPointsTexture(0),
     mPointsSize(0),
     mPointsProjection(0),
     mFlowUpdatePosShader(0),
     mFlowDepthTexture(0),
     mFlowVertexPosTexture(0),
     mFlowVertexParamTexture(0),
     mFlowFieldTexture(0),
     mFlowCenter(0),
     mFlowFieldScale(0),
     mFlowTimeDelta(0),
     mFlowSrcRect(0),
     mFlowInvProj(0),
     mFlowStop(0),
     mFlowInitPosShader(0),
     mFlowInitPosDepth(0),
     mFlowInitPosInvProj(0),
     mFlowInitSrcRect(0),
     mFlowInitParamShader(0),
     mFlowInitParamDepth(0),
     mFlowInitParamInvProj(0),
     mFlowInitParamCenter(0),
     mFlowUpdateColShader(0),
     mFlowUpdateColColorMap(0),
     mFlowUpdateColPositionMap(0),
     mFlowUpdateColProj(0),
     mFlowUpdateColStop(0),
     mDeferredBuffer(nullptr)
{
   Image image;
   image.load("data/game/flowfield_pointsprite");
   mParticleTextureId = activeDevice->createTexture(image.getData(), image.getWidth(), image.getHeight());

   image.load("data/game/flowfield");
   mFlowFieldTextureId = activeDevice->createTexture(image.getData(), image.getWidth(), image.getHeight(), 1);

   mPointsShader = activeDevice->loadShader("infectedparticles-vert.glsl", "infectedparticles-frag.glsl");
   mPointsTexture = activeDevice->getParameterIndex("texturemap");
   mPointsSize = activeDevice->getParameterIndex("particleSize");
   mPointsProjection = activeDevice->getParameterIndex("u_projection");

   mFlowUpdatePosShader = activeDevice->loadShader("infectedflowfield-vert.glsl", "infectedflowfield-frag.glsl");
   mFlowDepthTexture = activeDevice->getParameterIndex("depthTexture");
   mFlowVertexPosTexture = activeDevice->getParameterIndex("vertexPosTexture");
   mFlowVertexParamTexture = activeDevice->getParameterIndex("vertexParamTexture");
   mFlowFieldTexture = activeDevice->getParameterIndex("flowfieldTexture");
   mFlowCenter = activeDevice->getParameterIndex("center");
   mFlowFieldScale = activeDevice->getParameterIndex("fieldScale");
   mFlowTimeDelta = activeDevice->getParameterIndex("timeDelta");
   mFlowSrcRect = activeDevice->getParameterIndex("srcRect");
   mFlowInvProj = activeDevice->getParameterIndex("invProj");
   mFlowStop = activeDevice->getParameterIndex("stop");

   mFlowInitPosShader = activeDevice->loadShader("infectedinitpositions-vert.glsl", "infectedinitpositions-frag.glsl");
   mFlowInitPosDepth = activeDevice->getParameterIndex("depthmap");
   mFlowInitPosInvProj = activeDevice->getParameterIndex("invProj");
   mFlowInitSrcRect = activeDevice->getParameterIndex("srcRect");

   mFlowInitParamShader = activeDevice->loadShader("infectedinitparams-vert.glsl", "infectedinitparams-frag.glsl");
   mFlowInitParamDepth = activeDevice->getParameterIndex("depthmap");
   mFlowInitParamInvProj = activeDevice->getParameterIndex("invProj");
   mFlowInitParamCenter = activeDevice->getParameterIndex("center");

   mFlowUpdateColShader = activeDevice->loadShader("infectedupdatecolors-vert.glsl", "infectedupdatecolors-frag.glsl");
   mFlowUpdateColColorMap = activeDevice->getParameterIndex("colormap");
   mFlowUpdateColPositionMap = activeDevice->getParameterIndex("positionmap");
   mFlowUpdateColProj = activeDevice->getParameterIndex("proj");
   mFlowUpdateColStop = activeDevice->getParameterIndex("stop");
}

PlayerInfectedEffect::~PlayerInfectedEffect()
{
   clear();
   delete mDeferredBuffer;
}

void PlayerInfectedEffect::clear()
{
   mFlowAnimations.clear();
}

void PlayerInfectedEffect::add(Material* material)
{
   if (!material)
   {
      return;
   }

   Flow flow;
   flow.animation = std::make_unique<InfectedFlowFieldAnimation>();
   flow.material = material;
   mFlowAnimations.push_back(std::move(flow));
}

void PlayerInfectedEffect::remove(Material* material)
{
   for (auto& flow : mFlowAnimations)
   {
      if (flow.material == material)
      {
         flow.animation->stop();
      }
   }
}

void PlayerInfectedEffect::animate(float delta)
{
   mDelta = delta;

   for (auto it = mFlowAnimations.begin(); it != mFlowAnimations.end();)
   {
      if (it->animation->isElapsed())
      {
         it = mFlowAnimations.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

void PlayerInfectedEffect::render()
{
   Matrix proj = static_cast<GLDevice*>(activeDevice)->getProjectionMatrix();
   Matrix invProj = proj.invert4x4();

   const int width = activeDevice->getWidth();
   const int height = activeDevice->getHeight();

   if (!mDeferredBuffer)
   {
      mDeferredBuffer = new FrameBuffer(width, height, 0, FrameBuffer::DepthTexture);
   }
   else if (mDeferredBuffer->resolutionChanged(width, height))
   {
      mDeferredBuffer->setResolution(width, height);
   }

   FrameBuffer::push();

   glDepthMask(GL_FALSE);
   glDisable(GL_DEPTH_TEST);

   for (auto& flow : mFlowAnimations)
   {
      if (!flow.material)
      {
         continue;
      }

      // re-capture the infected player's current silhouette every frame - unlike PlayerDeathEffect
      // (a frozen one-shot capture), an infected player is still alive and moving.
      FrameBuffer::push(mDeferredBuffer);
      activeDevice->clear();

      Vector min;
      Vector max;
      flow.material->getBoundingRect(min, max, proj);
      Vector center = flow.material->getGeometry(0)->getTransform().translation();
      flow.animation->setCenter(center);

      flow.material->renderDiffuse();

      FrameBuffer::pop();

      if (!flow.animation->initialized())
      {
         flow.animation->initialize();
         flow.animation->setInitialized(true);

         activeDevice->setShader(mFlowInitPosShader);
         activeDevice->bindSampler(mFlowInitPosDepth, 0);
         activeDevice->setParameter(mFlowInitPosInvProj, invProj);
         activeDevice->setParameter(mFlowInitSrcRect, Vector4(min.x, min.y, max.x, max.y));
         flow.animation->initializePositions(mDeferredBuffer->depthTexture(), min, max);

         activeDevice->setShader(mFlowInitParamShader);
         activeDevice->bindSampler(mFlowInitParamDepth, 0);
         activeDevice->setParameter(mFlowInitParamInvProj, invProj);
         activeDevice->setParameter(mFlowInitParamCenter, Vector2(center.x, center.y));
         flow.animation->initializeParams(mDeferredBuffer->depthTexture(), min, max);

         activeDevice->setShader(0);
      }

      activeDevice->setShader(mFlowUpdatePosShader);
      activeDevice->bindSampler(mFlowDepthTexture, 0);
      activeDevice->bindSampler(mFlowVertexPosTexture, 1);
      activeDevice->bindSampler(mFlowVertexParamTexture, 2);
      activeDevice->bindSampler(mFlowFieldTexture, 3);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, mDeferredBuffer->depthTexture());

      glActiveTexture(GL_TEXTURE3);
      glBindTexture(GL_TEXTURE_2D, mFlowFieldTextureId);

      activeDevice->setParameter(mFlowCenter, flow.animation->getCenter());
      activeDevice->setParameter(mFlowFieldScale, flow.animation->getScale());
      activeDevice->setParameter(mFlowTimeDelta, mDelta);
      activeDevice->setParameter(mFlowSrcRect, Vector4(min.x, min.y, max.x, max.y));
      activeDevice->setParameter(mFlowInvProj, invProj);
      activeDevice->setParameter(mFlowStop, flow.animation->stopped() ? 1.0f : 0.0f);

      flow.animation->updatePositions(mDelta);

      activeDevice->setShader(mFlowUpdateColShader);
      activeDevice->bindSampler(mFlowUpdateColColorMap, 0);
      activeDevice->bindSampler(mFlowUpdateColPositionMap, 1);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, mDeferredBuffer->texture());

      activeDevice->setParameter(mFlowUpdateColProj, proj);
      activeDevice->setParameter(mFlowUpdateColStop, flow.animation->stopped() ? 1.0f : 0.0f);

      flow.animation->updateColors();

      activeDevice->setShader(0);
   }

   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
   glActiveTexture(GL_TEXTURE0);

   FrameBuffer::pop();

   // particle positions are already baked into world space - identity world transform, matching
   // PlayerDeathEffect::render()'s own convention for the same situation.
   activeDevice->setShader(mPointsShader);
   activeDevice->setParameter(mPointsProjection, proj);
   activeDevice->push(Matrix());

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_FALSE);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, mParticleTextureId);
   activeDevice->bindSampler(mPointsTexture, 0);

   float sizeFactor = 1.0f;
   FrameBuffer* fb = FrameBuffer::Instance();
   if (fb)
   {
      sizeFactor = fb->getSizeFactor(1920.0f);
   }

   for (const auto& flow : mFlowAnimations)
   {
      activeDevice->setParameter(mPointsSize, flow.animation->getPointSize() * sizeFactor);
      flow.animation->draw();
   }

   glDisable(GL_BLEND);
   glDepthMask(GL_TRUE);

   activeDevice->pop();
   activeDevice->setShader(0);
}
