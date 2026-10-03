// GLES3 port of client/src/game/countdowndrawable.cpp.

#include "countdowndrawable.h"

#include "menus/defaultshader.h"
#include "menus/psdlayer.h"

#include "framework/gldevice.h"

#include "math/matrix.h"

#define FADE_FACTOR 0.009f

CountdownDrawable::CountdownDrawable(RenderDevice* dev) : Drawable(*dev)
{
   _filename = "data/menus/countdown.psd";
}

CountdownDrawable::~CountdownDrawable() = default;

void CountdownDrawable::initializeGL()
{
   initializeLayers();

   _shader = getDefaultMenuShader();
}

void CountdownDrawable::paintGL()
{
   initGlParameters();
   drawCountdown();
   cleanupGlParameters();
}

void CountdownDrawable::animate(float time)
{
   _delta_time = time - _time;
   _time = time;

   if (!_delta_time_initialized)
   {
      _delta_time = 0.0f;
      _delta_time_initialized = true;
   }

   // decrease alpha
   bool done = true;
   for (auto& alpha : _layer_alphas)
   {
      if (alpha > 0.0f)
      {
         alpha = alpha - (FADE_FACTOR * _delta_time);
         done = false;
      }
   }

   // check if we're done
   if (done)
   {
      setVisible(false);
      _delta_time_initialized = false;
   }
}

void CountdownDrawable::drawCountdown()
{
   for (int layer_index = 0; layer_index < _psd_layers.size(); layer_index++)
   {
      if (_layer_alphas[layer_index] > 0.0f)
      {
         _psd_layers[layer_index]->render(0, 0, _layer_alphas[layer_index]);
      }
   }
}

void CountdownDrawable::countdown(int left)
{
   setVisible(true);
   _time_left = left;
   _layer_alphas[_layer_alphas.size() - 1 - left] = 1.0f;
}

void CountdownDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0.0f, _psd.getWidth(), _psd.getHeight(), 0.0f, -1.0f, 1.0f);
   static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(ortho);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   activeDevice().setShader(_shader);
}

void CountdownDrawable::cleanupGlParameters()
{
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void CountdownDrawable::initializeLayers()
{
   _psd.load(_filename.c_str());

   // assign layers to menu page items
   for (auto& psd_layer : _psd.getLayers())
   {
      _psd_layers.push_back(std::make_unique<PSDLayer>(psd_layer));
      _layer_alphas.push_back(0.0f);
   }
}
