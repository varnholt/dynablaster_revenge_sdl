#include "roundsdrawable.h"

#include "postproduction/motionblurfilter.h"

#include "framework/gldevice.h"
#include "math/matrix.h"
#include "math/vector2.h"
#include "menus/psdlayer.h"

#include "bombermanclient.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
constexpr float ANIMATION_DURATION = 7.0f * 62.5f;
constexpr float FADE_OUT_DURATION = 1.0f * 62.5f;
constexpr float BLUR_FACTOR = 0.15f;
}  // namespace

RoundsDrawable::RoundsDrawable(RenderDevice* dev) : Drawable(*dev)
{
   _filename = "data/game/rounds.psd";
   _motion_blur_filter = std::make_unique<MotionBlurFilter>();
}

RoundsDrawable::~RoundsDrawable()
{
}

void RoundsDrawable::initializeGL()
{
   initializeLayers();
   _motion_blur_filter->init();
}

void RoundsDrawable::paintGL()
{
   initGlParameters();

   _motion_blur_filter->setIntensity(1.0f);

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
   x = 0.75f * -speed * _psd.getWidth();

   GameInformation* info = BombermanClient::getInstance()->getCurrentGameInformation();

   if (info && info->getRoundCount() > 1)
   {
      if (info->getCurrentRound() == info->getRoundCount() - 1)
      {
         float scale = 1.0f / (_layer_round_final->getWidth() / _max_layer_width);
         _motion_blur_filter->setMotionDir(Vector2(speed * scale * BLUR_FACTOR, 0.0f));
         _motion_blur_filter->process(0, 0, 0);
         _layer_round_final->render(x, y, alpha);
      }
      else
      {
         PSDLayer* current_layer = nullptr;

         switch (info->getCurrentRound())
         {
            case 0:
               current_layer = _layer_round1;
               break;
            case 1:
               current_layer = _layer_round2;
               break;
            case 2:
               current_layer = _layer_round3;
               break;
            case 3:
               current_layer = _layer_round4;
               break;
            case 4:
               current_layer = _layer_round5;
               break;
            default:
               break;
         }

         if (!current_layer)
         {
            current_layer = _layer_round1;
         }

         float scale = 1.0f / (_layer_round->getWidth() / _max_layer_width);
         _motion_blur_filter->setMotionDir(Vector2(speed * scale * BLUR_FACTOR, 0.0f));
         _motion_blur_filter->process(0, 0, 0);
         _layer_round->render(x, y, alpha);

         if (current_layer)
         {
            scale = 1.0f / (current_layer->getWidth() / _max_layer_width);
            _motion_blur_filter->setMotionDir(Vector2(speed * scale * BLUR_FACTOR, 0.0f));
            _motion_blur_filter->process(0, 0, 0);
            current_layer->render(x, y, alpha);
         }
      }
   }

   activeDevice().setShader(0);

   cleanupGlParameters();
}

void RoundsDrawable::animate(float time)
{
   _time = time;

   if (_initialize_time)
   {
      _start_time = time;
      _initialize_time = false;
   }

   if (getRelativeTime() > ANIMATION_DURATION + FADE_OUT_DURATION)
   {
      setVisible(false);
   }
}

void RoundsDrawable::showGame()
{
   setVisible(true);
   _initialize_time = true;
}

void RoundsDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0.0f, _psd.getWidth(), _psd.getHeight(), 0.0f, -1.0f, 1.0f);
   static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(ortho);

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
   return _time - _start_time;
}

void RoundsDrawable::initializeLayers()
{
   _psd.load(_filename.c_str());

   for (auto& psd_layer : _psd.getLayers())
   {
      auto layer = std::make_unique<PSDLayer>(psd_layer);

      _max_layer_width = std::max(_max_layer_width, static_cast<float>(layer->getWidth()));

      if (psd_layer.getName() == "round_label")
      {
         _layer_round = layer.get();
      }
      else if (psd_layer.getName() == "finalround_label")
      {
         _layer_round_final = layer.get();
      }
      else if (psd_layer.getName() == "r1")
      {
         _layer_round1 = layer.get();
      }
      else if (psd_layer.getName() == "r2")
      {
         _layer_round2 = layer.get();
      }
      else if (psd_layer.getName() == "r3")
      {
         _layer_round3 = layer.get();
      }
      else if (psd_layer.getName() == "r4")
      {
         _layer_round4 = layer.get();
      }
      else if (psd_layer.getName() == "r5")
      {
         _layer_round5 = layer.get();
      }

      _psd_layers.push_back(std::move(layer));
   }
}
