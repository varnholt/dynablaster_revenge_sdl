#include "renderdevice.h"

// static render-device object
RenderDevice* activeDevice = nullptr;

RenderDevice::RenderDevice()
{
   activeDevice = this;
}

int32_t RenderDevice::getBorderLeft() const
{
   return _border_left;
}

int32_t RenderDevice::getBorderBottom() const
{
   return _border_bottom;
}

void RenderDevice::setKey(int32_t num, int32_t state)
{
   _keys[num] = state;
}

int32_t RenderDevice::getKey(int32_t num)
{
   return _keys[num];
}

void RenderDevice::setAbort()
{
   _abort = 1;
}

void RenderDevice::setActive(bool state)
{
   _active = state;
}

bool RenderDevice::active()
{
   return (_active != 0);
}

bool RenderDevice::abort()
{
   return (_abort != 0);
}

Matrix RenderDevice::getCameraMatrix() const
{
   return _camera;
}

void RenderDevice::setBorder(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
   _border_left = left;
   _border_top = top;
   _border_right = right;
   _border_bottom = bottom;
}

float RenderDevice::getWidth() const
{
   return static_cast<float>(_width) - _border_left - _border_right;
}

float RenderDevice::getHeight() const
{
   return static_cast<float>(_height) - _border_top - _border_bottom;
}
