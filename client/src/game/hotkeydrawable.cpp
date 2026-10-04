#include "hotkeydrawable.h"

#include "framework/gldevice.h"
#include "math/matrix.h"
#include "menus/defaultshader.h"
#include "menus/psdlayer.h"

#include <algorithm>

namespace
{
constexpr float FADE_TIME = 500.0f;
}

HotkeyDrawable::HotkeyDrawable(RenderDevice& dev) : Drawable(dev)
{
}

HotkeyDrawable::~HotkeyDrawable()
{
}

void HotkeyDrawable::initializeGL()
{
   _psd.load("data/game/help.psd");

   for (auto& psd_layer : _psd.getLayers())
   {
      _psd_layers.push_back(std::make_unique<PSDLayer>(psd_layer));
   }
}

float HotkeyDrawable::computeAlpha() const
{
   const float alpha = std::min(_animation_timer.elapsed() / FADE_TIME, 1.0f);
   return _show ? alpha : 1.0f - alpha;
}

void HotkeyDrawable::paintGL()
{
   const float alpha = computeAlpha();

   if (!_show && alpha <= 0.0f)
   {
      Drawable::setVisible(false);
      return;
   }

   initGlParameters();

   for (const auto& layer : _psd_layers)
   {
      if (layer->getLayer().isVisible())
      {
         layer->render(0.0f, 0.0f, alpha);
      }
   }

   cleanupGlParameters();
}

void HotkeyDrawable::setVisible(bool visible)
{
   _animation_timer.restart();
   _show = visible;

   // hiding fades out first, paintGL() hides the drawable once that's done
   if (visible)
   {
      Drawable::setVisible(true);
   }
}

bool HotkeyDrawable::isShown() const
{
   return _show;
}

void HotkeyDrawable::toggle()
{
   if (!isVisible())
   {
      setVisible(true);
   }
   else if (isShown())
   {
      setVisible(false);
   }
}

void HotkeyDrawable::initGlParameters()
{
   Matrix ortho = Matrix::ortho(0.0f, _psd.getWidth(), _psd.getHeight(), 0.0f, -1.0f, 1.0f);
   static_cast<GLDevice&>(activeDevice()).setProjectionMatrix(ortho);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   activeDevice().setShader(getDefaultMenuShader());
}

void HotkeyDrawable::cleanupGlParameters()
{
   activeDevice().setShader(0);

   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}
