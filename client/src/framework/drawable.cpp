#include "drawable.h"

Drawable::Drawable(RenderDevice* device, bool visible) : _device(device), _visible(visible)
{
}

void Drawable::animate(float /*global_time*/)
{
}

void Drawable::resizeGL()
{
}

void Drawable::mousePressEvent(int32_t /*x*/, int32_t /*y*/)
{
}

void Drawable::mouseMoveEvent(int32_t /*x*/, int32_t /*y*/)
{
}

void Drawable::mouseReleaseEvent()
{
}

void Drawable::keyPressEvent(const KeyEvent& /*event*/)
{
}

void Drawable::keyReleaseEvent(const KeyEvent& /*event*/)
{
}

void Drawable::setVisible(bool visible)
{
   _visible = visible;
}

bool Drawable::isVisible() const
{
   return _visible;
}
