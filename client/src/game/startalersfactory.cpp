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
   float randX = frand(1.0f) * x;
   float randY = frand(1.0f) * y;

   Vector dir(randX, randY, 0.75f);
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

StarTalersFactory::Burst::Burst(const Vector& fieldPosition, const Vector& color)
 : mVertexBuffer(0),
   mIndexBuffer(0),
   mIndexCount(0),
   mTime(0.0f),
   mFieldPosition(fieldPosition),
   mColor(color)
{
   int vertexCount = STAR_COUNT * 12;
   mIndexCount = STAR_COUNT * 18;

   mVertexBuffer = activeDevice->createVertexBuffer(vertexCount * static_cast<int>(sizeof(StarTalersVertex)));
   mIndexBuffer = activeDevice->createIndexBuffer(mIndexCount * static_cast<int>(sizeof(unsigned short)));

   auto* vtx = static_cast<StarTalersVertex*>(activeDevice->lockVertexBuffer(mVertexBuffer));

   const float scale = 0.5f;
   int index = 0;

   for (int i = 0; i < vertexCount; i += 12)
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

      const float us[12] = {u + 0.5f, u + 0.0f, u + 0.0f, u + 0.5f, u + 0.5f, u + 0.0f, u + 0.0f, u + 0.5f, u + 0.5f, u + 0.0f, u + 0.0f, u + 0.5f};
      const float vs[12] = {v + 0.5f, v + 0.5f, v + 0.0f, v + 0.0f, v + 0.5f, v + 0.5f, v + 0.0f, v + 0.0f, v + 0.5f, v + 0.5f, v + 0.0f, v + 0.0f};

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

   activeDevice->unlockVertexBuffer(mVertexBuffer);

   auto* idx = static_cast<unsigned short*>(activeDevice->lockIndexBuffer(mIndexBuffer));

   index = 0;
   for (int i = 0; i < mIndexCount; i += 18)
   {
      idx[i] = static_cast<unsigned short>(0 + index);
      idx[i + 1] = static_cast<unsigned short>(1 + index);
      idx[i + 2] = static_cast<unsigned short>(3 + index);

      idx[i + 3] = static_cast<unsigned short>(3 + index);
      idx[i + 4] = static_cast<unsigned short>(1 + index);
      idx[i + 5] = static_cast<unsigned short>(2 + index);

      idx[i + 6] = static_cast<unsigned short>(4 + index);
      idx[i + 7] = static_cast<unsigned short>(5 + index);
      idx[i + 8] = static_cast<unsigned short>(7 + index);

      idx[i + 9] = static_cast<unsigned short>(7 + index);
      idx[i + 10] = static_cast<unsigned short>(5 + index);
      idx[i + 11] = static_cast<unsigned short>(6 + index);

      idx[i + 12] = static_cast<unsigned short>(8 + index);
      idx[i + 13] = static_cast<unsigned short>(9 + index);
      idx[i + 14] = static_cast<unsigned short>(11 + index);

      idx[i + 15] = static_cast<unsigned short>(11 + index);
      idx[i + 16] = static_cast<unsigned short>(9 + index);
      idx[i + 17] = static_cast<unsigned short>(10 + index);

      index += 12;
   }

   activeDevice->unlockIndexBuffer(mIndexBuffer);
}

StarTalersFactory::Burst::~Burst()
{
   activeDevice->deleteBuffer(mVertexBuffer);
   activeDevice->deleteBuffer(mIndexBuffer);
}

bool StarTalersFactory::Burst::isElapsed() const
{
   return mTime > DURATION;
}

void StarTalersFactory::Burst::update(float dt)
{
   mTime += dt;
}

void StarTalersFactory::Burst::render(int fieldParam, int colorParam, int timeParam)
{
   activeDevice->setParameter(timeParam, mTime);
   activeDevice->setParameter(fieldParam, mFieldPosition);
   activeDevice->setParameter(colorParam, mColor);

   glBindBuffer(GL_ARRAY_BUFFER, mVertexBuffer);

   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glEnableVertexAttribArray(3);
   glEnableVertexAttribArray(4);

   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), (GLvoid*)offsetof(StarTalersVertex, position));
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), (GLvoid*)offsetof(StarTalersVertex, normal));
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), (GLvoid*)offsetof(StarTalersVertex, u));
   glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), (GLvoid*)offsetof(StarTalersVertex, direction));
   glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(StarTalersVertex), (GLvoid*)offsetof(StarTalersVertex, speed));

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
   glDrawElements(GL_TRIANGLES, mIndexCount, GL_UNSIGNED_SHORT, 0);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(3);
   glDisableVertexAttribArray(4);
}

StarTalersFactory::StarTalersFactory()
 : mShader(0),
   mTextureId(0),
   mFieldParam(-1),
   mColorParam(-1),
   mTimeParam(-1),
   mCameraParam(-1),
   mTextureParam(-1)
{
   mColorBomb.set(0.047f, 0.49f, 0.012f);
   mColorFlame.set(1.0f, 0.518f, 0.0f);
   mColorSpeedup.set(0.0f, 0.588f, 1.0f);
   mColorKick.set(0.635f, 0.0f, 1.0f);
   mColorDefault.set(1.0f, 1.0f, 1.0f);
}

StarTalersFactory::~StarTalersFactory()
{
   if (mTextureId)
   {
      activeDevice->deleteTexture(mTextureId);
   }
}

void StarTalersFactory::initialize()
{
   Image image;
   image.load("data/effects/startalers/startalers_particles");
   mTextureId = activeDevice->createTexture(image.getData(), image.getWidth(), image.getHeight());

   mShader = activeDevice->loadShader("startalers-vert.glsl", "startalers-frag.glsl");
   mFieldParam = activeDevice->getParameterIndex("field");
   mColorParam = activeDevice->getParameterIndex("color");
   mTimeParam = activeDevice->getParameterIndex("time");
   mCameraParam = activeDevice->getParameterIndex("camera");
   mTextureParam = activeDevice->getParameterIndex("texturemap");
}

const Vector& StarTalersFactory::getColor(Constants::ExtraType extra) const
{
   switch (extra)
   {
      case Constants::ExtraBomb:
         return mColorBomb;
      case Constants::ExtraFlame:
         return mColorFlame;
      case Constants::ExtraSpeedup:
         return mColorSpeedup;
      case Constants::ExtraKick:
         return mColorKick;
      default:
         return mColorDefault;
   }
}

void StarTalersFactory::add(float x, float y, Constants::ExtraType extra)
{
   Vector fieldPosition(x + 0.5f, -(y + 0.5f), 0.0f);
   mBursts.push_back(std::make_unique<Burst>(fieldPosition, getColor(extra)));
}

void StarTalersFactory::update(float dt)
{
   if (dt <= 0.0f)
   {
      return;
   }

   for (auto& burst : mBursts)
   {
      burst->update(dt);
   }

   std::erase_if(mBursts, [](const std::unique_ptr<Burst>& burst) { return burst->isElapsed(); });
}

void StarTalersFactory::render()
{
   if (mBursts.empty())
   {
      return;
   }

   glDisable(GL_CULL_FACE);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   activeDevice->setShader(mShader);

   Vector camera = static_cast<GLDevice*>(activeDevice)->getProjectionMatrix().z();
   activeDevice->setParameter(mCameraParam, camera);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, mTextureId);
   activeDevice->bindSampler(mTextureParam, 0);

   activeDevice->push(Matrix());

   for (auto& burst : mBursts)
   {
      burst->render(mFieldParam, mColorParam, mTimeParam);
   }

   activeDevice->pop();

   activeDevice->setShader(0);

   glDepthMask(GL_TRUE);
   glEnable(GL_CULL_FACE);
   glDisable(GL_BLEND);
}
