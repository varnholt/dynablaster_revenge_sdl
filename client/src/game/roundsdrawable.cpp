#include "roundsdrawable.h"

#include "postproduction/motionblurfilter.h"

#include "framework/gldevice.h"
#include "menus/psdlayer.h"
#include "math/matrix.h"
#include "math/vector2.h"

#include "bombermanclient.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

namespace
{
constexpr float ANIMATION_DURATION = 7.0f * 62.5f;
constexpr float FADE_OUT_DURATION = 1.0f * 62.5f;
constexpr float BLUR_FACTOR = 0.15f;
}  // namespace

RoundsDrawable::RoundsDrawable(RenderDevice* dev)
   : Drawable(dev),
     mLayerRound(nullptr),
     mLayerRoundFinal(nullptr),
     mLayerRound1(nullptr),
     mLayerRound2(nullptr),
     mLayerRound3(nullptr),
     mLayerRound4(nullptr),
     mLayerRound5(nullptr),
     mTime(0.0f),
     mStartTime(0.0f),
     mInitializeTime(false),
     mMotionBlurFilter(nullptr),
     mMaxLayerWidth(0.0f)
{
   mFilename = "data/game/rounds.psd";
   mMotionBlurFilter = new MotionBlurFilter();
}

RoundsDrawable::~RoundsDrawable()
{
   delete mMotionBlurFilter;
}

void RoundsDrawable::initializeGL()
{
   initializeLayers();
   mMotionBlurFilter->init();
}

void RoundsDrawable::paintGL()
{
   initGlParameters();

   mMotionBlurFilter->setIntensity(1.0f);

   float x = 0.0f;
   float y = 0.0f;
   float alpha = 1.0f;

   float t = getRelativeTime() * 0.016f;

   float t1 = 2.0f;
   float t2 = 4.0f;
   float t3 = 6.0f;

   float speed = 0.0f;
   if (t < t1)
   {
      speed = 0.5f * (std::cos(t * (1.0f / t1) * std::numbers::pi_v<float>) + 1.0f);
   }
   else if (t < t2)
   {
      speed = 0.0f;
   }
   else if (t < t3)
   {
      speed = -(1.0f - (0.5f * (std::cos((t2 - t) * (1.0f / (t3 - t2)) * std::numbers::pi_v<float>) + 1.0f)));
   }
   else
   {
      speed = -1.0f;
   }

   speed *= std::fabs(speed);
   x = 0.75f * -speed * mPsd.getWidth();

   GameInformation* info = BombermanClient::getInstance()->getCurrentGameInformation();

   if (info && info->getRoundCount() > 1)
   {
      if (info->getCurrentRound() == info->getRoundCount() - 1)
      {
         float scale = 1.0f / (mLayerRoundFinal->getWidth() / mMaxLayerWidth);
         mMotionBlurFilter->setMotionDir(Vector2(speed * scale * BLUR_FACTOR, 0.0f));
         mMotionBlurFilter->process(0, 0, 0);
         mLayerRoundFinal->render(x, y, alpha);
      }
      else
      {
         PSDLayer* currentLayer = nullptr;

         switch (info->getCurrentRound())
         {
            case 0:
               currentLayer = mLayerRound1;
               break;
            case 1:
               currentLayer = mLayerRound2;
               break;
            case 2:
               currentLayer = mLayerRound3;
               break;
            case 3:
               currentLayer = mLayerRound4;
               break;
            case 4:
               currentLayer = mLayerRound5;
               break;
            default:
               break;
         }

         if (!currentLayer)
         {
            currentLayer = mLayerRound1;
         }

         float scale = 1.0f / (mLayerRound->getWidth() / mMaxLayerWidth);
         mMotionBlurFilter->setMotionDir(Vector2(speed * scale * BLUR_FACTOR, 0.0f));
         mMotionBlurFilter->process(0, 0, 0);
         mLayerRound->render(x, y, alpha);

         if (currentLayer)
         {
            scale = 1.0f / (currentLayer->getWidth() / mMaxLayerWidth);
            mMotionBlurFilter->setMotionDir(Vector2(speed * scale * BLUR_FACTOR, 0.0f));
            mMotionBlurFilter->process(0, 0, 0);
            currentLayer->render(x, y, alpha);
         }
      }
   }

   activeDevice->setShader(0);

   cleanupGlParameters();
}

void RoundsDrawable::animate(float time)
{
   mTime = time;

   if (mInitializeTime)
   {
      mStartTime = time;
      mInitializeTime = false;
   }

   if (getRelativeTime() > ANIMATION_DURATION + FADE_OUT_DURATION)
   {
      setVisible(false);
   }
}

void RoundsDrawable::showGame()
{
   setVisible(true);
   mInitializeTime = true;
}

void RoundsDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0.0f, mPsd.getWidth(), mPsd.getHeight(), 0.0f, -1.0f, 1.0f);
   static_cast<GLDevice*>(activeDevice)->setProjectionMatrix(ortho);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
}

void RoundsDrawable::cleanupGlParameters()
{
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

float RoundsDrawable::getRelativeTime() const
{
   return mTime - mStartTime;
}

void RoundsDrawable::initializeLayers()
{
   mPsd.load(mFilename.c_str());

   for (int l = 0; l < mPsd.getLayerCount(); l++)
   {
      PSD::Layer* psdLayer = mPsd.getLayer(l);
      auto layer = std::make_unique<PSDLayer>(psdLayer);

      mMaxLayerWidth = std::max(mMaxLayerWidth, static_cast<float>(layer->getWidth()));

      if (std::strcmp(psdLayer->getName(), "round_label") == 0)
      {
         mLayerRound = layer.get();
      }
      else if (std::strcmp(psdLayer->getName(), "finalround_label") == 0)
      {
         mLayerRoundFinal = layer.get();
      }
      else if (std::strcmp(psdLayer->getName(), "r1") == 0)
      {
         mLayerRound1 = layer.get();
      }
      else if (std::strcmp(psdLayer->getName(), "r2") == 0)
      {
         mLayerRound2 = layer.get();
      }
      else if (std::strcmp(psdLayer->getName(), "r3") == 0)
      {
         mLayerRound3 = layer.get();
      }
      else if (std::strcmp(psdLayer->getName(), "r4") == 0)
      {
         mLayerRound4 = layer.get();
      }
      else if (std::strcmp(psdLayer->getName(), "r5") == 0)
      {
         mLayerRound5 = layer.get();
      }

      mPsdLayers.push_back(std::move(layer));
   }
}
