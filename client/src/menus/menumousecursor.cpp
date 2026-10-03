#include "menumousecursor.h"

#include "defaultshader.h"
#include "framework/gldevice.h"
#include "math/matrix.h"
#include "math/quat.h"
#include "psdlayer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
const std::string LAYER_DEFAULT = "default";
const std::string LAYER_BUSY = "busy";
const std::string LAYER_CLICKED = "clicked";
}  // namespace

MenuMouseCursor::MenuMouseCursor(RenderDevice& device, bool visible) : Drawable(device, visible)
{
}

MenuMouseCursor::~MenuMouseCursor() = default;

void MenuMouseCursor::animate(float time)
{
   _time = time;
}

void MenuMouseCursor::mousePressEvent(int /*x*/, int /*y*/)
{
   _mouse_pressed = true;
   _click_time.restart();
}

void MenuMouseCursor::mouseReleaseEvent()
{
   _mouse_pressed = false;
}

void MenuMouseCursor::mouseMoveEvent(int x, int y)
{
   _x = x;
   _y = y;
}

void MenuMouseCursor::setBusy(bool busy)
{
   _busy = busy;
}

void MenuMouseCursor::initializeGL()
{
   initializeLayers();
}

void MenuMouseCursor::initializeLayers()
{
   _psd.load(_filename.c_str());

   const auto end = _psd.getLayers().end();

   if (const auto psd_layer = _psd.getLayer(LAYER_DEFAULT); psd_layer != end)
   {
      _default_layer = std::make_unique<PSDLayer>(*psd_layer);
   }

   if (const auto psd_layer = _psd.getLayer(LAYER_CLICKED); psd_layer != end)
   {
      _clicked_layer = std::make_unique<PSDLayer>(*psd_layer);
   }

   if (const auto psd_layer = _psd.getLayer(LAYER_BUSY); psd_layer != end)
   {
      _busy_x = psd_layer->getLeft();
      _busy_y = psd_layer->getTop();
      psd_layer->setX(0);
      psd_layer->setY(0);
      _busy_layer = std::make_unique<PSDLayer>(*psd_layer);
   }
}

void MenuMouseCursor::paintGL()
{
   initGlParameters();

   paintDefaultCursor();
   paintClickedCursor();
   paintBusyIcon();

   cleanupGlParameters();
}

void MenuMouseCursor::paintCursor(PSDLayer& layer, float opacity)
{
   layer.render(_x, _y, opacity);
}

void MenuMouseCursor::paintDefaultCursor()
{
   if (_default_layer)
   {
      paintCursor(*_default_layer);
   }
}

void MenuMouseCursor::paintClickedCursor()
{
   if (!_clicked_layer)
   {
      return;
   }

   const float opacity = std::max(300 - static_cast<int>(_click_time.elapsed()), 0) * 0.00003f * _clicked_layer->getOpacity();

   if (opacity > 0.0f)
   {
      paintCursor(*_clicked_layer, opacity);
   }
}

void MenuMouseCursor::paintBusyIcon()
{
   if (!_busy)
   {
      return;
   }

   // rotate the icon around its own center, then move it to (x + busy_x, y + busy_y).
   // Not visually verified yet, nothing sets the busy state so far.
   const float w = static_cast<float>(_busy_layer->getWidth());
   const float h = static_cast<float>(_busy_layer->getHeight());
   const float angle = _time * std::numbers::pi_v<float> / 180.0f;

   Matrix pre;
   pre.translate(Vector(-0.5f * w, -0.5f * h, 0.0f));

   const Matrix rotation(Quat(0.0f, 0.0f, std::sin(angle * 0.5f), std::cos(angle * 0.5f)));

   Matrix post;
   post.translate(Vector(_x + _busy_x + 0.5f * w, _y + _busy_y + 0.5f * h, 0.0f));

   const Matrix world = pre * rotation * post;
   activeDevice().push(world);

   _busy_layer->render();

   activeDevice().pop();
}

void MenuMouseCursor::cleanupGlParameters()
{
   glDisable(GL_BLEND);
   glEnable(GL_DEPTH_TEST);
   glDepthMask(GL_TRUE);
}

void MenuMouseCursor::initGlParameters()
{
   auto& device = static_cast<GLDevice&>(activeDevice());

   glEnable(GL_BLEND);

   glDisable(GL_DEPTH_TEST);
   glDepthMask(GL_FALSE);

   // shader and projection must be set before push(), which uploads the combined MVP immediately;
   // PSDLayer::render() needs the shared menu shader bound (shader 0 means "no program" in GLES3)
   activeDevice().setShader(getDefaultMenuShader());
   device.setProjectionMatrix(Matrix::ortho(0.0f, 1920.0f, 1080.0f, 0.0f, -1.0f, 1.0f));
   device.push(Matrix());
}
