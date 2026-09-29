#include "clipper.h"
#include "framework/framebuffer.h"
#include "framework/gldevice.h"

#include <cmath>

Clipper::Clipper(float left, float top, float right, float bottom) : _left(left), _top(top), _right(right), _bottom(bottom)
{
}

float Clipper::getWidth() const
{
   return _right - _left;
}

float Clipper::getHeight() const
{
   return _bottom - _top;
}

void Clipper::setBounds(float left, float top, float right, float bottom)
{
   _left = left;
   _top = top;
   _right = right;
   _bottom = bottom;
}

int Clipper::getClipFlags(const Vertex& vertex) const
{
   int clip = 0;

   if (vertex.x < _left)
   {
      clip |= Left;
   }
   if (vertex.x > _right)
   {
      clip |= Right;
   }
   if (vertex.y < _top)
   {
      clip |= Top;
   }
   if (vertex.y > _bottom)
   {
      clip |= Bottom;
   }

   return clip;
}

Array<Vertex> Clipper::clipLeft(const Array<Vertex>& vertices)
{
   Array<Vertex> result;
   const int size = vertices.size();

   if (size <= 0)
   {
      return result;
   }

   int c1 = getClipFlags(vertices[size - 1]) & Left;
   const Vertex* v1 = &vertices[size - 1];

   for (int i = 0; i < size; i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Left;
      const Vertex* v2 = &vertices[i];

      if (!c1)
      {
         result.add(*v1);
      }

      if (c1 != c2)
      {
         // v1.x + (v2.x - v1.x)*t = _left
         const float t = (_left - v1->x) / (v2->x - v1->x);
         result.add(*v1 + (*v2 - *v1) * t);
      }

      v1 = v2;
      c1 = c2;
   }

   return result;
}

Array<Vertex> Clipper::clipRight(const Array<Vertex>& vertices)
{
   Array<Vertex> result;
   const int size = vertices.size();

   if (size <= 0)
   {
      return result;
   }

   int c1 = getClipFlags(vertices[size - 1]) & Right;
   const Vertex* v1 = &vertices[size - 1];

   for (int i = 0; i < size; i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Right;
      const Vertex* v2 = &vertices[i];

      if (!c1)
      {
         result.add(*v1);
      }

      if (c1 != c2)
      {
         // v1.x + (v2.x - v1.x)*t = _right
         const float t = (_right - v1->x) / (v2->x - v1->x);
         result.add(*v1 + (*v2 - *v1) * t);
      }

      v1 = v2;
      c1 = c2;
   }

   return result;
}

Array<Vertex> Clipper::clipTop(const Array<Vertex>& vertices)
{
   Array<Vertex> result;
   const int size = vertices.size();

   if (size <= 0)
   {
      return result;
   }

   int c1 = getClipFlags(vertices[size - 1]) & Top;
   const Vertex* v1 = &vertices[size - 1];

   for (int i = 0; i < size; i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Top;
      const Vertex* v2 = &vertices[i];

      if (!c1)
      {
         result.add(*v1);
      }

      if (c1 != c2)
      {
         // v1.y + (v2.y - v1.y)*t = _top
         const float t = (_top - v1->y) / (v2->y - v1->y);
         result.add(*v1 + (*v2 - *v1) * t);
      }

      v1 = v2;
      c1 = c2;
   }

   return result;
}

Array<Vertex> Clipper::clipBottom(const Array<Vertex>& vertices)
{
   Array<Vertex> result;
   const int size = vertices.size();

   if (size <= 0)
   {
      return result;
   }

   int c1 = getClipFlags(vertices[size - 1]) & Bottom;
   const Vertex* v1 = &vertices[size - 1];

   for (int i = 0; i < size; i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Bottom;
      const Vertex* v2 = &vertices[i];

      if (!c1)
      {
         result.add(*v1);
      }

      if (c1 != c2)
      {
         // v1.y + (v2.y - v1.y)*t = _bottom
         const float t = (_bottom - v1->y) / (v2->y - v1->y);
         result.add(*v1 + (*v2 - *v1) * t);
      }

      v1 = v2;
      c1 = c2;
   }

   return result;
}

Array<Vertex> Clipper::clip(const Array<Vertex>& vertices)
{
   Array<Vertex> left = clipLeft(vertices);
   Array<Vertex> right = clipRight(left);
   Array<Vertex> top = clipTop(right);
   return clipBottom(top);
}

bool Clipper::visible(const Array<Vertex>& vertices) const
{
   int flags = -1;
   const int size = vertices.size();
   for (int i = 0; i < size; i++)
   {
      flags &= getClipFlags(vertices[i]);
   }
   return flags == 0;
}

bool Clipper::enable(const Array<Vertex>& vertices)
{
   int all_flags = 0;
   int any_flags = Left | Right | Top | Bottom;
   const int size = vertices.size();
   for (int i = 0; i < size; i++)
   {
      const int flag = getClipFlags(vertices[i]);
      all_flags |= flag;
      any_flags &= flag;
   }

   // some flag is set for all vertices: the polygon is completely outside the clip area
   if (any_flags != 0)
   {
      return false;
   }

   // at least one vertex is outside the clip area: enable clipping
   if (all_flags != 0)
   {
      enable();
   }

   return true;
}

void Clipper::enable()
{
   if (_enabled)
   {
      return;
   }

   _enabled = true;

   int width = 0;
   int height = 0;
   if (FrameBuffer* frame_buffer = FrameBuffer::Instance())
   {
      width = frame_buffer->width();
      height = frame_buffer->height();
   }
   else
   {
      width = static_cast<int>(activeDevice->getWidth());
      height = static_cast<int>(activeDevice->getHeight());
   }

   glScissor(
      static_cast<int>(std::floor(_left * width / _screen_width)),
      static_cast<int>(std::floor((_screen_height - _bottom - 1) * height / _screen_height)),
      static_cast<int>(std::ceil((_right - _left) * width / _screen_width) + 1),
      static_cast<int>(std::ceil((_bottom - _top) * height / _screen_height) + 1)
   );
   glEnable(GL_SCISSOR_TEST);
}

void Clipper::disable()
{
   if (_enabled)
   {
      _enabled = false;
      glDisable(GL_SCISSOR_TEST);
      glScissor(0, 0, static_cast<int>(activeDevice->getWidth()), static_cast<int>(activeDevice->getHeight()));
   }
}
