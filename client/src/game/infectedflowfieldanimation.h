#pragma once

#include "math/vector.h"
#include "math/vector4.h"

class InfectedFlowFieldAnimation
{
public:
   InfectedFlowFieldAnimation();
   ~InfectedFlowFieldAnimation();

   void initialize();
   void initializePositions(unsigned int depthMap, const Vector& min, const Vector& max);
   void initializeParams(unsigned int depthMap, const Vector& min, const Vector& max);

   bool initialized() const;
   void setInitialized(bool init);

   const Vector& getCenter() const;
   void setCenter(const Vector& center);

   float getScale() const;

   //! return size of particles in unprojected units
   float getPointSize() const;

   void updatePositions(float deltaTime);
   void updateColors();

   void draw();

   //! all particles have faded away
   bool isElapsed() const;

   //! stop respawning of particles
   void stop();
   bool stopped() const;

private:
   bool mInitialized;
   int mWidth;
   int mHeight;
   unsigned int mVertexPosBuffer;
   unsigned int mVertexColorBuffer;
   unsigned int mVertexParamTexture;
   unsigned int mParamTarget;
   unsigned int mVertexColorTexture;
   unsigned int mColorTarget;
   unsigned int mPositions[2];
   unsigned int mPosTarget[2];
   int mPage;

   Vector mCenter;
   float mFlowScale;
   float mParticleSize;

   float mElapsed;
   bool mStop;
};
