// header
#include "gamelogodrawable.h"
#include "gamelogopointsprite.h"
#include "menus/defaultshader.h"
#include "menus/psdlayer.h"

// engine
#include "gldevice.h"
#include "math/matrix.h"
#include "tools/random.h"

// cmath
#include <cmath>
#include <cstring>
// the real page-name string MenuDrawable's pageChangedSignal carries for the actual
// main menu page (client/src/menus/gamemenudefines.h's MAINMENU - that header itself isn't
// ported, it's all networking/gameplay action-name constants unrelated to rendering).
#define MAINMENU "data/menus/mainmenu.psd"

#define SPARK_COUNT 300
#define SPARK_START_INTENSITY 0.06f

#define LAYER_DYNABLASTER "dynablaster"
#define LAYER_REVENGE "revenge"

#define FADE_IN_LENGTH 80.0f
#define FADE_OUT_LENGTH 80.0f

GameLogoDrawable::GameLogoDrawable(RenderDevice* dev, bool visible) : SphereFragmentsDrawable(dev, visible)
{
   _filename = "data/logo/logo.psd";
}

GameLogoDrawable::~GameLogoDrawable() = default;

void GameLogoDrawable::initializeGL()
{
   initializeLayers();
   initializeSparks();

   SphereFragmentsDrawable::initializeGL();
}

void GameLogoDrawable::paintGL()
{
   updateFadeAlpha();

   if (_alpha > 0.0f)
   {
      SphereFragmentsDrawable::paintGL();

      initOrthoGlParameters();

      _layer_dynablaster->render(20.0f * std::cos(_time * 0.03f), 30.0f + 15.0f * std::sin(_time * 0.04f), _alpha);

      _layer_revenge->render(30.0f * std::cos(_time * 0.03f), 30.0f + 25.0f * std::sin(_time * 0.04f), _alpha);

      initPointSpriteGlParameters();
      drawSparks();

      cleanupGlParameters();
   }
}

void GameLogoDrawable::pageChanged(const std::string& page)
{
   bool was_visible = _main_menu_visible;
   _main_menu_visible = (page == MAINMENU);

   // fade out
   if (was_visible && !_main_menu_visible)
   {
      _fade_out_end = _time + FADE_OUT_LENGTH;
   }

   // fade in
   else if (!was_visible && _main_menu_visible)
   {
      _fade_in_end = _time + FADE_IN_LENGTH;
   }
}

void GameLogoDrawable::animate(float time)
{
   _delta_time = time - _time;
   _time = time;
}

void GameLogoDrawable::setVisible(bool visible)
{
   Drawable::setVisible(visible);

   if (!visible)
      _spark_times_initialized = false;
}

void GameLogoDrawable::drawSparks()
{
   bool visible = true;

   Array<Vector> positions;
   Array<float> glow;

   if (!_spark_times_initialized)
   {
      for (int i = 0; i < _sparks.size(); i++)
         _sparks[i]._start_time = _time + frand(100.0f);

      _spark_times_initialized = true;
   }

   for (int i = 0; i < _sparks.size(); i++)
   {
      if (_sparks[i]._start_time <= _time)
      {
         // reset spark if intensity threshold exceeded
         if (_sparks[i]._intensity < 0.01f)
         {
            initSpark(_sparks[i]);
            _sparks[i]._start_time = _time + frand(100.0f);
         }

         _sparks[i]._direction.y -= _delta_time * 0.02f;
         _sparks[i]._scalar += _delta_time * 0.02f;
         _sparks[i]._intensity -= _delta_time * 0.0015f;

         Vector pos = _sparks[i]._origin + (_sparks[i]._direction * _sparks[i]._length * _sparks[i]._scalar);

         // TODO: fix this (kept verbatim from the original - see gamelogodrawable.cpp)
         pos.z = -12.0f;

         // spark fading
         float intensity = 1.0f;
         if (_fade_in_end > _time)
         {
            intensity = 1.0f - (_fade_in_end - _time) / FADE_IN_LENGTH;
         }
         else if (_fade_out_end > _time)
         {
            intensity = (_fade_out_end - _time) / FADE_OUT_LENGTH;
         }
         else if (!_main_menu_visible)
         {
            visible = false;
         }

         if (visible)
         {
            positions.add(pos);
            glow.add(_sparks[i]._intensity * intensity);
         }
      }
   }

   if (visible)
   {
      GameLogoPointSprite::setPointSprites(positions, glow);

      GameLogoPointSprite::draw();
   }
}

void GameLogoDrawable::updateFadeAlpha()
{
   float alpha = _main_menu_visible ? 1.0f : 0.0f;

   if (_fade_in_end > _time)
   {
      alpha = 1.0f - (_fade_in_end - _time) / FADE_IN_LENGTH;
   }
   else if (_fade_out_end > _time)
   {
      alpha = (_fade_out_end - _time) / FADE_OUT_LENGTH;
   }

   _alpha = alpha;
}

void GameLogoDrawable::initOrthoGlParameters()
{
   static_cast<GLDevice*>(_device)->setProjectionMatrix(
      Matrix::ortho(0.0f, (float)_psd.getWidth(), (float)_psd.getHeight(), 0.0f, 300.0f, -300.0f)
   );

   // PSDLayer::render() (see menus/psdlayer.cpp) draws through whichever shader the caller has
   // already bound - matches the same convention MenuDrawable/MenuPage established for every
   // other PSD-backed item draw.
   _device->setShader(getDefaultMenuShader());

   // enable blending
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
}

void GameLogoDrawable::initPointSpriteGlParameters()
{
   float znear = 0.1f;
   float zfar = 5000.0f;

   float scale = 0.5f;
   float aspect = 16.0f / 9.0f;

   float ymin = znear * scale;
   float ymax = -ymin;

   float xmax = ymax * aspect;
   float xmin = ymin * aspect;

   static_cast<GLDevice*>(_device)->setProjectionMatrix(Matrix::frustum(xmin, xmax, ymin, ymax, znear, zfar));

   // init blending (additive glow)
   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE);

   // GameLogoPointSprite::draw() binds its own shader.
   _device->setShader(0);
}

void GameLogoDrawable::cleanupGlParameters()
{
   // enable blending
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GameLogoDrawable::initializeLayers()
{
   _psd.load(_filename.c_str());

   // assign layers to the two named pointers this class actually renders (see the header
   // comment - "earth"/"highlight" layers are still constructed here, matching the original 1:1,
   // even though nothing renders them: harmless data loading, not GL-drawing code, so left as-is
   // per this port's own scope rule).
   for (int l = 0; l < _psd.getLayerCount(); l++)
   {
      PSD::Layer* psdlayer = _psd.getLayer(l);

      auto owned_layer = std::make_unique<PSDLayer>(psdlayer);
      PSDLayer* layer = owned_layer.get();

      if (std::strcmp(psdlayer->getName(), LAYER_DYNABLASTER) == 0)
      {
         _layer_dynablaster = layer;
      }
      else if (std::strcmp(psdlayer->getName(), LAYER_REVENGE) == 0)
      {
         _layer_revenge = layer;
      }

      _layers.push_back(std::move(owned_layer));
   }
}

void GameLogoDrawable::initSpark(Spark& spark)
{
   spark._origin = _spark_origin;  // maybe vary a little
   spark._position = _spark_origin;
   spark._intensity = SPARK_START_INTENSITY;
   spark._scalar = 0.0f;

   float origin_rand_x = frand(0.2f);
   float origin_rand_y = frand(0.2f);
   float origin_rand_z = -0.2f + frand(0.2f);

   spark._origin.x += origin_rand_x;
   spark._origin.y += origin_rand_y;
   spark._origin.z += origin_rand_z;

   float dir_rand_x = -0.75f + frand(0.5f);
   float dir_rand_y = -0.75f + frand(0.5f);

   spark._direction = Vector(-1.0f - dir_rand_x, -1.0f - dir_rand_y, -12.0f);

   spark._length = 1.0f;
}

void GameLogoDrawable::initializeSparks()
{
   GameLogoPointSprite::initialize();

   _spark_origin = Vector(-3.7f, -3.3f, -12.0f);

   for (int i = 0; i < SPARK_COUNT; i++)
   {
      Spark spark;
      initSpark(spark);
      _sparks.add(spark);
   }
}
