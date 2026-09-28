#include "playerinvincibleeffect.h"
#include "playerinvincibleinstance.h"

#include "framework/framebuffer.h"
#include "framework/gldevice.h"
#include "framework/globaltime.h"
#include "materials/material.h"
#include "math/matrix.h"
#include "math/vector2.h"
#include "math/vector4.h"
#include "nodes/mesh.h"
#include "nodes/node.h"
#include "render/geometry.h"
#include "render/texturepool.h"

#include <cmath>

namespace
{
// draws a quad (as 2 triangles) with an explicit position component count (2 for NDC-space
// blur-pass quads, 3 for the world-space displace-pass quad) plus an optional uv pair.
void drawQuad(const float* verts, int posComponents, int uvComponents)
{
   const int floatsPerVertex = posComponents + uvComponents;
   const int order[6] = {0, 1, 2, 0, 2, 3};
   float buffer[6 * 5];

   for (int i = 0; i < 6; i++)
   {
      const float* src = verts + order[i] * floatsPerVertex;
      float* dst = buffer + i * floatsPerVertex;
      for (int c = 0; c < floatsPerVertex; c++)
      {
         dst[c] = src[c];
      }
   }

   static unsigned int quadVertexBuffer = 0;
   if (quadVertexBuffer == 0)
   {
      glGenBuffers(1, &quadVertexBuffer);
   }

   glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * floatsPerVertex, buffer, GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, posComponents, GL_FLOAT, GL_FALSE, sizeof(float) * floatsPerVertex, (GLvoid*)0);

   if (uvComponents > 0)
   {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, uvComponents, GL_FLOAT, GL_FALSE, sizeof(float) * floatsPerVertex, (GLvoid*)(sizeof(float) * posComponents));
   }

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   if (uvComponents > 0)
   {
      glDisableVertexAttribArray(1);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// perspective-corrected back-projection of a 2d screen rect into 3d world space, using a
// reference triangle placed in front of the player as the known world<->screen correspondence.
void backProject(Vector* dst, const Vector& min2d, const Vector& max2d, const Matrix& objMat, const Matrix& projMat)
{
   struct Vertex
   {
      float x, y, z;
      float u, v, w;
   };

   Vector pos = objMat.translation();

   Vector worldTri[3];
   worldTri[0] = Vector(-0.6f, -0.5f, 2.0f) + pos;
   worldTri[1] = Vector(-0.6f, -0.5f, 0.0f) + pos;
   worldTri[2] = Vector(0.6f, -0.5f, 0.0f) + pos;

   Vertex vtx[3];
   const Matrix& mat = projMat;
   for (int i = 0; i < 3; i++)
   {
      const Vector& v = worldTri[i];
      float x = mat.xx * v.x + mat.xy * v.y + mat.xz * v.z + mat.xw;
      float y = mat.yx * v.x + mat.yy * v.y + mat.yz * v.z + mat.yw;
      float w = mat.wx * v.x + mat.wy * v.y + mat.wz * v.z + mat.ww;
      float t = 1.0f / w;
      vtx[i].x = x * t;
      vtx[i].y = y * t;
      vtx[i].z = t;
      vtx[i].u = v.x * t;
      vtx[i].v = v.y * t;
      vtx[i].w = v.z * t;
   }

   float delta1 = vtx[1].y - vtx[2].y;
   float delta2 = vtx[0].y - vtx[2].y;
   float denom = (vtx[0].x - vtx[2].x) * delta1 - (vtx[1].x - vtx[2].x) * delta2;

   denom = 1.0f / denom;
   delta1 *= denom;
   delta2 *= denom;

   float deltau = (vtx[0].u - vtx[2].u) * delta1 - (vtx[1].u - vtx[2].u) * delta2;
   float deltav = (vtx[0].v - vtx[2].v) * delta1 - (vtx[1].v - vtx[2].v) * delta2;
   float deltaw = (vtx[0].w - vtx[2].w) * delta1 - (vtx[1].w - vtx[2].w) * delta2;
   float deltaz = (vtx[0].z - vtx[2].z) * delta1 - (vtx[1].z - vtx[2].z) * delta2;

   int minVtx = 0;
   for (int n = 1; n < 3; n++)
   {
      if (vtx[n].y < vtx[minVtx].y)
      {
         minVtx = n;
      }
   }

   Vertex* v1 = &vtx[minVtx];
   minVtx++;
   if (minVtx > 2)
   {
      minVtx = 0;
   }
   Vertex* v2 = &vtx[minVtx];
   float invHeight = 1.0f / (v2->y - v1->y);
   float leftDx = (v2->x - v1->x) * invHeight;
   float leftDz = (v2->z - v1->z) * invHeight;
   float leftDu = (v2->u - v1->u) * invHeight;
   float leftDv = (v2->v - v1->v) * invHeight;
   float leftDw = (v2->w - v1->w) * invHeight;

   Vector rect2d[4];
   rect2d[0] = Vector(min2d.x, min2d.y);
   rect2d[1] = Vector(max2d.x, min2d.y);
   rect2d[2] = Vector(max2d.x, max2d.y);
   rect2d[3] = Vector(min2d.x, max2d.y);

   for (int i = 0; i < 4; i++)
   {
      float dx = rect2d[i].x - v1->x;
      float dy = rect2d[i].y - v1->y;

      float u = v1->u + dy * leftDu - dy * leftDx * deltau + dx * deltau;
      float v = v1->v + dy * leftDv - dy * leftDx * deltav + dx * deltav;
      float w = v1->w + dy * leftDw - dy * leftDx * deltaw + dx * deltaw;
      float z = v1->z + dy * leftDz - dy * leftDx * deltaz + dx * deltaz;

      float t = 1.0f / z;

      dst[i].x = u * t;
      dst[i].y = v * t;
      dst[i].z = w * t;
   }
}
}  // namespace

PlayerInvincibleEffect::PlayerInvincibleEffect()
   : mRadius(0.0f),
     mBlurHShader(0),
     mBlurHTexture(-1),
     mBlurHTexelOffset(-1),
     mBlurHRadius(-1),
     mBlurHKernel(-1),
     mBlurVShader(0),
     mBlurVTexture(-1),
     mBlurVTexelOffset(-1),
     mBlurVRadius(-1),
     mBlurVKernel(-1),
     mDisplaceShader(0),
     mDisplaceTexture1(-1),
     mDisplaceTexture2(-1),
     mDisplaceTexelOffset(-1),
     mDisplaceOffset(-1),
     mDisplaceSourceRect(-1),
     mDisplaceFade(-1),
     mDisplayCenter(-1),
     mScratchBuffer(nullptr)
{
   mDisplacementTexture = TexturePool::Instance()->getTexture("data/game/displace");

   setRadius(10.0f);

   mBlurHShader = activeDevice->loadShader("playerinvincibleblurh-vert.glsl", "playerinvincibleblurh-frag.glsl");
   mBlurHTexture = activeDevice->getParameterIndex("texturemap");
   mBlurHTexelOffset = activeDevice->getParameterIndex("texelOffset");
   mBlurHRadius = activeDevice->getParameterIndex("radius");
   mBlurHKernel = activeDevice->getParameterIndex("kernel");

   mBlurVShader = activeDevice->loadShader("playerinvincibleblurv-vert.glsl", "playerinvincibleblurv-frag.glsl");
   mBlurVTexture = activeDevice->getParameterIndex("texturemap");
   mBlurVTexelOffset = activeDevice->getParameterIndex("texelOffset");
   mBlurVRadius = activeDevice->getParameterIndex("radius");
   mBlurVKernel = activeDevice->getParameterIndex("kernel");

   mDisplaceShader = activeDevice->loadShader("playerinvincibledisplace-vert.glsl", "playerinvincibledisplace-frag.glsl");
   mDisplaceTexture1 = activeDevice->getParameterIndex("texturemap");
   mDisplaceTexture2 = activeDevice->getParameterIndex("displace");
   mDisplaceTexelOffset = activeDevice->getParameterIndex("texelOffset");
   mDisplaceOffset = activeDevice->getParameterIndex("displaceOffset");
   mDisplaceSourceRect = activeDevice->getParameterIndex("sourceRect");
   mDisplaceFade = activeDevice->getParameterIndex("fade");
   mDisplayCenter = activeDevice->getParameterIndex("center");
}

PlayerInvincibleEffect::~PlayerInvincibleEffect()
{
   clear();
   delete mScratchBuffer;
}

void PlayerInvincibleEffect::setRadius(float radius)
{
   if (radius > 30.0f)
   {
      radius = 30.0f;
   }

   mRadius = radius;
   int size = static_cast<int>(std::ceil(mRadius));
   const double scale = -4.0 / (mRadius * mRadius);
   float sum = 0.0f;
   for (int i = 0; i <= size; i++)
   {
      float f = static_cast<float>(std::pow(2.718281828459045, i * i * scale));
      sum += f;
      mKernel[i] = f;
   }
   for (int i = size + 1; i < 32; i++)
   {
      mKernel[i] = 0.0f;
   }

   float t = 1.1f / (sum * 2.0f - 1.0f);
   for (int i = 0; i <= size; i++)
   {
      mKernel[i] *= t;
   }
}

void PlayerInvincibleEffect::clear()
{
   mPlayers.clear();
}

void PlayerInvincibleEffect::add(Material* material)
{
   if (!material)
   {
      return;
   }

   auto it = mPlayers.find(material);
   if (it == mPlayers.end())
   {
      auto player = std::make_unique<PlayerInvincibleInstance>();
      player->setMaterial(material);
      mPlayers[material] = std::move(player);
   }
   else
   {
      it->second->setRemove(false);
   }
}

void PlayerInvincibleEffect::remove(Material* playerMaterial)
{
   auto it = mPlayers.find(playerMaterial);
   if (it != mPlayers.end())
   {
      it->second->remove();
   }
}

void PlayerInvincibleEffect::setMaterialFade(PlayerInvincibleInstance* player, float fade)
{
   Material* mat = player->getMaterial();
   if (!mat)
   {
      return;
   }

   Geometry* geo = mat->getGeometry(0);
   if (!geo)
   {
      return;
   }

   Mesh* mesh = static_cast<Mesh*>(geo->getParent());
   if (mesh)
   {
      mesh->setRenderParameter(2, fade);
   }
}

void PlayerInvincibleEffect::animate(float dt)
{
   for (auto it = mPlayers.begin(); it != mPlayers.end();)
   {
      PlayerInvincibleInstance* player = it->second.get();

      if (!player->update(dt))
      {
         setMaterialFade(player, 0.0f);
         it = mPlayers.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

void PlayerInvincibleEffect::blurPlayers(FrameBuffer* temp, const Matrix& proj)
{
   FrameBuffer::push();

   for (auto& [material, player] : mPlayers)
   {
      Material* mat = player->getMaterial();
      if (!mat)
      {
         continue;
      }

      Vector min2d(0.0f, 0.0f, 0.0f);
      Vector max2d(0.0f, 0.0f, 0.0f);

      mat->getBoundingRect(min2d, max2d, proj);

      float bx = mRadius * 2.0f / temp->width();
      float by = mRadius * 2.0f / temp->height();
      min2d.x -= bx;
      min2d.y -= by;
      max2d.x += bx;
      max2d.y += by;

      player->setRect(min2d, max2d);
      player->setCenter(mat->getCenter2d(proj));

      min2d.x = (min2d.x + 1.0f) * 0.5f;
      min2d.y = (min2d.y + 1.0f) * 0.5f;
      max2d.x = (max2d.x + 1.0f) * 0.5f;
      max2d.y = (max2d.y + 1.0f) * 0.5f;

      temp->bind();
      activeDevice->clear();
      setMaterialFade(player, player->getFade());
      mat->renderDiffuse();
      temp->unbind();

      // horizontal blur - temp framebuffer into the player's own ping-pong texture 0
      player->bind(0);
      activeDevice->clear();
      activeDevice->setShader(mBlurHShader);
      glBindTexture(GL_TEXTURE_2D, temp->texture());
      activeDevice->bindSampler(mBlurHTexture, 0);
      activeDevice->setParameter(mBlurHTexelOffset, Vector2(1.0f / temp->width()));
      activeDevice->setParameter(mBlurHRadius, mRadius);
      activeDevice->setParameter(mBlurHKernel, mKernel, 32);

      float x = (max2d.x - min2d.x) * 2.0f * temp->width() / player->width();
      float y = (max2d.y - min2d.y) * 2.0f * temp->height() / player->height();

      const float quadH[4 * 4] = {
         -1.0f, -1.0f, min2d.x, min2d.y, -1.0f + x, -1.0f, max2d.x, min2d.y,
         -1.0f + x, -1.0f + y, max2d.x, max2d.y, -1.0f, -1.0f + y, min2d.x, max2d.y,
      };
      drawQuad(quadH, 2, 2);

      player->unbind();

      // vertical blur - texture 0 into texture 1
      player->bind(1);
      activeDevice->clear();
      activeDevice->setShader(mBlurVShader);
      glBindTexture(GL_TEXTURE_2D, player->texture(0));
      activeDevice->setParameter(mBlurVTexelOffset, Vector2(0.0f, 1.0f / player->height()));
      activeDevice->setParameter(mBlurVRadius, mRadius);
      activeDevice->setParameter(mBlurVKernel, mKernel, 32);

      const float quadV[4 * 4] = {
         -1.0f, -1.0f, 0.0f, 0.0f, -1.0f + x, -1.0f, x * 0.5f, 0.0f,
         -1.0f + x, -1.0f + y, x * 0.5f, y * 0.5f, -1.0f, -1.0f + y, 0.0f, y * 0.5f,
      };
      drawQuad(quadV, 2, 2);

      player->unbind();

      activeDevice->setShader(0);
   }

   FrameBuffer::pop();
}

void PlayerInvincibleEffect::render()
{
   if (mPlayers.empty())
   {
      return;
   }

   Matrix proj = static_cast<GLDevice*>(activeDevice)->getProjectionMatrix();

   float time = GlobalTime::Instance()->getTime() * 0.1f;

   const int width = activeDevice->getWidth();
   const int height = activeDevice->getHeight();

   if (!mScratchBuffer)
   {
      mScratchBuffer = new FrameBuffer(width, height, 0, FrameBuffer::NoDepthBuffer);
   }
   else if (mScratchBuffer->resolutionChanged(width, height))
   {
      mScratchBuffer->setResolution(width, height);
   }

   blurPlayers(mScratchBuffer, proj);

   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice->setShader(mDisplaceShader);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, mDisplacementTexture);
   activeDevice->bindSampler(mDisplaceTexture2, 1);

   glActiveTexture(GL_TEXTURE0);
   activeDevice->bindSampler(mDisplaceTexture1, 0);

   for (auto& [material, player] : mPlayers)
   {
      Material* mat = player->getMaterial();
      if (!mat)
      {
         continue;
      }

      Geometry* geo = mat->getGeometry(0);
      if (!geo)
      {
         continue;
      }

      const Matrix& tm = geo->getTransform();

      Vector min2d = player->min2d();
      Vector max2d = player->max2d();

      activeDevice->setParameter(mDisplaceTexelOffset, Vector2(mScratchBuffer->width() / 1920.0f, mScratchBuffer->height() / 1080.0f));

      activeDevice->setParameter(
         mDisplaceSourceRect,
         Vector4(min2d.x, min2d.y, 0.5f * mScratchBuffer->width() / player->width(), 0.5f * mScratchBuffer->height() / player->height())
      );

      activeDevice->setParameter(
         mDisplaceOffset,
         Vector4(
            std::sin(time * 2.69 * 0.025) * 0.5f * 1920.0f / mScratchBuffer->width(),
            -(time * 0.5f + std::sin(time * 2.81 * 0.025) * 0.5f) * 1920.0f / mScratchBuffer->width(),
            -std::cos(time * 3.11 * 0.025) * 0.5f * 1920.0f / mScratchBuffer->width(),
            -(time * 0.5f - std::cos(time * 3.59 * 0.025) * 0.5f) * 1920.0f / mScratchBuffer->width()
         )
      );

      activeDevice->setParameter(mDisplaceFade, player->getFade());
      activeDevice->setParameter(mDisplayCenter, player->getCenter());

      Vector pos[4];
      backProject(pos, min2d, max2d, tm, proj);

      glBindTexture(GL_TEXTURE_2D, player->texture(1));

      const float quad[4 * 3] = {
         pos[0].x, pos[0].y, pos[0].z, pos[1].x, pos[1].y, pos[1].z, pos[2].x, pos[2].y, pos[2].z, pos[3].x, pos[3].y, pos[3].z,
      };
      drawQuad(quad, 3, 0);
   }

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, 0);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glDisable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDepthMask(GL_TRUE);

   activeDevice->setShader(0);
}
