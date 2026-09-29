#include "drawable.h"

Drawable::Drawable(RenderDevice* device, bool visible) : mDevice(device), mVisible(visible)
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
   mVisible = visible;
}

bool Drawable::isVisible() const
{
   return mVisible;
}
