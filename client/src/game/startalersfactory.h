#pragma once

/// \brief bonus-pickup coin/star burst, shown when a player collects an extra.

#include <memory>
#include <vector>

#include "math/vector.h"

// shared
#include "constants.h"

class StarTalersFactory
{
public:
   StarTalersFactory();
   ~StarTalersFactory();

   void initialize();
   void add(float x, float y, Constants::ExtraType extra);
   void update(float dt);
   void render();

private:
   class Burst
   {
   public:
      Burst(const Vector& fieldPosition, const Vector& color);
      ~Burst();

      void update(float dt);
      void render(int fieldParam, int colorParam, int timeParam);
      bool isElapsed() const;

   private:
      unsigned int mVertexBuffer;
      unsigned int mIndexBuffer;
      int mIndexCount;
      float mTime;
      Vector mFieldPosition;
      Vector mColor;
   };

   const Vector& getColor(Constants::ExtraType extra) const;

   std::vector<std::unique_ptr<Burst>> mBursts;

   unsigned int mShader;
   unsigned int mTextureId;
   int mFieldParam;
   int mColorParam;
   int mTimeParam;
   int mCameraParam;
   int mTextureParam;

   Vector mColorBomb;
   Vector mColorFlame;
   Vector mColorSpeedup;
   Vector mColorKick;
   Vector mColorDefault;
};
