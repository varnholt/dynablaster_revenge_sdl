#pragma once

#include "drawable.h"

#include <memory>
#include <string>
#include <vector>

#include "image/psd.h"

class MotionBlurFilter;
class PSDLayer;

class RoundsDrawable : public Drawable
{
public:
   RoundsDrawable(RenderDevice* dev);
   virtual ~RoundsDrawable();

   void initializeGL();
   void paintGL();
   void animate(float time);

   void showGame();

protected:
   void initializeLayers();
   void initGlParameters();
   void cleanupGlParameters();
   float getRelativeTime() const;

   PSD mPsd;
   std::vector<std::unique_ptr<PSDLayer>> mPsdLayers;

   PSDLayer* mLayerRound;
   PSDLayer* mLayerRoundFinal;
   PSDLayer* mLayerRound1;
   PSDLayer* mLayerRound2;
   PSDLayer* mLayerRound3;
   PSDLayer* mLayerRound4;
   PSDLayer* mLayerRound5;

   std::string mFilename;

   float mTime;
   float mStartTime;
   bool mInitializeTime;

   MotionBlurFilter* mMotionBlurFilter;

   float mMaxLayerWidth;
};
