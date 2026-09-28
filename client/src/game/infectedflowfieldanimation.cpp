#include "infectedflowfieldanimation.h"

#include "framework/gldevice.h"
#include "framework/framebuffer.h"

namespace
{
constexpr int PARTICLE_GRID_WIDTH = 64;
constexpr int PARTICLE_GRID_HEIGHT = 48;
constexpr float PARTICLE_SIZE = 60.0f;
constexpr float DISSOLVE_TIME = 300.0f;

GLuint gQuadVertexBuffer = 0;

void drawQuad(const float* verts, int floatsPerVertex)
{
   const int order[6] = {0, 1, 2, 0, 2, 3};
   float buffer[6 * 4];

   for (int i = 0; i < 6; i++)
   {
      const float* src = verts + order[i] * floatsPerVertex;
      float* dst = buffer + i * floatsPerVertex;
      for (int c = 0; c < floatsPerVertex; c++)
      {
         dst[c] = src[c];
      }
   }

   if (gQuadVertexBuffer == 0)
   {
      glGenBuffers(1, &gQuadVertexBuffer);
   }

   glBindBuffer(GL_ARRAY_BUFFER, gQuadVertexBuffer);
   glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * floatsPerVertex, buffer, GL_DYNAMIC_DRAW);

   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * floatsPerVertex, (GLvoid*)0);

   if (floatsPerVertex > 2)
   {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * floatsPerVertex, (GLvoid*)(sizeof(float) * 2));
   }

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   if (floatsPerVertex > 2)
   {
      glDisableVertexAttribArray(1);
   }

   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
}  // namespace

InfectedFlowFieldAnimation::InfectedFlowFieldAnimation()
   : mInitialized(false),
     mWidth(0),
     mHeight(0),
     mVertexPosBuffer(0),
     mVertexColorBuffer(0),
     mVertexParamTexture(0),
     mParamTarget(0),
     mVertexColorTexture(0),
     mColorTarget(0),
     mPage(0),
     mCenter(0.0f, 0.0f, 0.0f),
     mFlowScale(0.2f),
     mParticleSize(0.0f),
     mElapsed(0.0f),
     mStop(false)
{
   mPositions[0] = mPositions[1] = 0;
   mPosTarget[0] = mPosTarget[1] = 0;
}

InfectedFlowFieldAnimation::~InfectedFlowFieldAnimation()
{
   glDeleteFramebuffers(1, &mParamTarget);
   glDeleteFramebuffers(1, &mColorTarget);
   glDeleteFramebuffers(1, &mPosTarget[0]);
   glDeleteFramebuffers(1, &mPosTarget[1]);
   glDeleteTextures(1, &mVertexParamTexture);
   glDeleteTextures(1, &mVertexColorTexture);
   glDeleteTextures(1, &mPositions[0]);
   glDeleteTextures(1, &mPositions[1]);
   glDeleteBuffers(1, &mVertexPosBuffer);
   glDeleteBuffers(1, &mVertexColorBuffer);
}

bool InfectedFlowFieldAnimation::initialized() const
{
   return mInitialized;
}

void InfectedFlowFieldAnimation::setInitialized(bool init)
{
   mInitialized = init;
}

const Vector& InfectedFlowFieldAnimation::getCenter() const
{
   return mCenter;
}

void InfectedFlowFieldAnimation::setCenter(const Vector& center)
{
   mCenter = center;
}

float InfectedFlowFieldAnimation::getScale() const
{
   return mFlowScale;
}

float InfectedFlowFieldAnimation::getPointSize() const
{
   return mParticleSize;
}

void InfectedFlowFieldAnimation::stop()
{
   mStop = true;
}

bool InfectedFlowFieldAnimation::stopped() const
{
   return mStop;
}

bool InfectedFlowFieldAnimation::isElapsed() const
{
   return mElapsed > DISSOLVE_TIME;
}

void InfectedFlowFieldAnimation::initialize()
{
   mWidth = PARTICLE_GRID_WIDTH;
   mHeight = PARTICLE_GRID_HEIGHT;
   mParticleSize = PARTICLE_SIZE;

   glGenTextures(1, &mVertexParamTexture);
   glBindTexture(GL_TEXTURE_2D, mVertexParamTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &mParamTarget);
   glBindFramebuffer(GL_FRAMEBUFFER, mParamTarget);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mVertexParamTexture, 0);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   glGenTextures(1, &mVertexColorTexture);
   glBindTexture(GL_TEXTURE_2D, mVertexColorTexture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, 0);
   glBindTexture(GL_TEXTURE_2D, 0);

   glGenFramebuffers(1, &mColorTarget);
   glBindFramebuffer(GL_FRAMEBUFFER, mColorTarget);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mVertexColorTexture, 0);
   glClear(GL_COLOR_BUFFER_BIT);
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

   for (int i = 0; i < 2; i++)
   {
      glGenTextures(1, &mPositions[i]);
      glBindTexture(GL_TEXTURE_2D, mPositions[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, 0);
      glBindTexture(GL_TEXTURE_2D, 0);
   }

   for (int i = 0; i < 2; i++)
   {
      glGenFramebuffers(1, &mPosTarget[i]);
      glBindFramebuffer(GL_FRAMEBUFFER, mPosTarget[i]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mPositions[i], 0);
      glClear(GL_COLOR_BUFFER_BIT);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
   }

   glGenBuffers(1, &mVertexPosBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, mVertexPosBuffer);
   glBufferData(GL_ARRAY_BUFFER, mWidth * mHeight * sizeof(Vector4), 0, GL_DYNAMIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);

   glGenBuffers(1, &mVertexColorBuffer);
   glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
   glBufferData(GL_ARRAY_BUFFER, mWidth * mHeight * sizeof(Vector4), 0, GL_DYNAMIC_DRAW);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InfectedFlowFieldAnimation::initializePositions(unsigned int depthMap, const Vector& min, const Vector& max)
{
   glBindFramebuffer(GL_FRAMEBUFFER, mPosTarget[1]);
   glViewport(0, 0, mWidth, mHeight);
   glBindTexture(GL_TEXTURE_2D, depthMap);

   const float quad[4 * 4] = {
      -1.0f, -1.0f, min.x, min.y, 1.0f, -1.0f, max.x, min.y, 1.0f, 1.0f, max.x, max.y, -1.0f, 1.0f, min.x, max.y,
   };
   drawQuad(quad, 4);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::initializeParams(unsigned int depthMap, const Vector& min, const Vector& max)
{
   glBindFramebuffer(GL_FRAMEBUFFER, mParamTarget);
   glViewport(0, 0, mWidth, mHeight);
   glBindTexture(GL_TEXTURE_2D, depthMap);

   const float quad[4 * 4] = {
      -1.0f, -1.0f, min.x, min.y, 1.0f, -1.0f, max.x, min.y, 1.0f, 1.0f, max.x, max.y, -1.0f, 1.0f, min.x, max.y,
   };
   drawQuad(quad, 4);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::updatePositions(float delta)
{
   if (mStop)
   {
      mElapsed += delta;
   }

   glBindFramebuffer(GL_FRAMEBUFFER, mPosTarget[mPage]);
   glViewport(0, 0, mWidth, mHeight);
   mPage ^= 1;

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, mPositions[mPage]);

   glActiveTexture(GL_TEXTURE2);
   glBindTexture(GL_TEXTURE_2D, mVertexParamTexture);

   glActiveTexture(GL_TEXTURE0);

   const float quad[4 * 2] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
   drawQuad(quad, 2);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::updateColors()
{
   glBindFramebuffer(GL_FRAMEBUFFER, mColorTarget);
   glViewport(0, 0, mWidth, mHeight);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, mPositions[1 - mPage]);
   glActiveTexture(GL_TEXTURE0);

   // a particle only gets a fresh color the frame it respawns (see infectedupdatecolors-frag.glsl)
   // - blending preserves every other particle's existing color instead of blanking it each frame.
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   const float quad[4 * 2] = {-1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f};
   drawQuad(quad, 2);

   glDisable(GL_BLEND);

   glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void InfectedFlowFieldAnimation::draw()
{
   FrameBuffer* prev = FrameBuffer::Instance();

   glBindFramebuffer(GL_FRAMEBUFFER, mColorTarget);
   glViewport(0, 0, mWidth, mHeight);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, mVertexColorBuffer);
   glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_FLOAT, 0);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

   glBindFramebuffer(GL_FRAMEBUFFER, mPosTarget[1 - mPage]);
   glViewport(0, 0, mWidth, mHeight);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, mVertexPosBuffer);
   glReadPixels(0, 0, mWidth, mHeight, GL_RGBA, GL_FLOAT, 0);
   glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

   if (prev)
   {
      glBindFramebuffer(GL_FRAMEBUFFER, prev->target());
      glViewport(0, 0, prev->width(), prev->height());
   }
   else
   {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(activeDevice->getBorderLeft(), activeDevice->getBorderBottom(), activeDevice->getWidth(), activeDevice->getHeight());
   }

   glBindBuffer(GL_ARRAY_BUFFER, mVertexPosBuffer);
   glEnableVertexAttribArray(0);
   glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (GLvoid*)0);

   glBindBuffer(GL_ARRAY_BUFFER, mVertexColorBuffer);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vector4), (GLvoid*)0);

   glDrawArrays(GL_POINTS, 0, mWidth * mHeight);

   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);
   glBindBuffer(GL_ARRAY_BUFFER, 0);
}
