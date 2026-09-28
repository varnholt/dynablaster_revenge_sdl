#pragma once

#include "math/vector.h"
#include "render/texture.h"

#include <unordered_map>

class Material;
class Matrix;
class FrameBuffer;
class PlayerInvincibleInstance;

class PlayerInvincibleEffect
{
public:
   PlayerInvincibleEffect();
   ~PlayerInvincibleEffect();

   void clear();
   void setRadius(float radius);

   void add(Material* playerMaterial);
   void remove(Material* playerMaterial);
   void animate(float dt);
   void render();

private:
   void blurPlayers(FrameBuffer* dst, const Matrix& proj);
   void setMaterialFade(PlayerInvincibleInstance* player, float fade);

   std::unordered_map<Material*, PlayerInvincibleInstance*> mPlayers;
   Texture mDisplacementTexture;
   float mRadius;
   float mKernel[32];

   unsigned int mBlurHShader;
   int mBlurHTexture;
   int mBlurHTexelOffset;
   int mBlurHRadius;
   int mBlurHKernel;

   unsigned int mBlurVShader;
   int mBlurVTexture;
   int mBlurVTexelOffset;
   int mBlurVRadius;
   int mBlurVKernel;

   unsigned int mDisplaceShader;
   int mDisplaceTexture1;
   int mDisplaceTexture2;
   int mDisplaceTexelOffset;
   int mDisplaceOffset;
   int mDisplaceSourceRect;
   int mDisplaceFade;
   int mDisplayCenter;

   FrameBuffer* mScratchBuffer;
};
