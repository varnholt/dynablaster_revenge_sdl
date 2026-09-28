#pragma once

#include <memory>
#include <vector>

class InfectedFlowFieldAnimation;
class Material;
class Vector;
class Matrix;
class FrameBuffer;

class PlayerInfectedEffect
{
public:
   PlayerInfectedEffect();
   ~PlayerInfectedEffect();

   void clear();

   void add(Material* playerMaterial);
   void remove(Material* playerMaterial);

   void animate(float delta);
   void render();

private:
   struct Flow
   {
      std::unique_ptr<InfectedFlowFieldAnimation> animation;
      Material* material;
   };

   std::vector<Flow> mFlowAnimations;

   float mDelta;

   unsigned int mParticleTextureId;
   unsigned int mFlowFieldTextureId;

   unsigned int mPointsShader;
   int mPointsTexture;
   int mPointsSize;
   int mPointsProjection;

   unsigned int mFlowUpdatePosShader;
   int mFlowDepthTexture;
   int mFlowVertexPosTexture;
   int mFlowVertexParamTexture;
   int mFlowFieldTexture;
   int mFlowCenter;
   int mFlowFieldScale;
   int mFlowTimeDelta;
   int mFlowSrcRect;
   int mFlowInvProj;
   int mFlowStop;

   unsigned int mFlowInitPosShader;
   int mFlowInitPosDepth;
   int mFlowInitPosInvProj;
   int mFlowInitSrcRect;

   unsigned int mFlowInitParamShader;
   int mFlowInitParamDepth;
   int mFlowInitParamInvProj;
   int mFlowInitParamCenter;

   unsigned int mFlowUpdateColShader;
   int mFlowUpdateColColorMap;
   int mFlowUpdateColPositionMap;
   int mFlowUpdateColProj;
   int mFlowUpdateColStop;

   // owns its own offscreen render target for capturing an infected player's color+depth each
   // frame - see FuseParticleSystem/PlayerDeathEffect for the same established pattern.
   FrameBuffer* mDeferredBuffer;
};
