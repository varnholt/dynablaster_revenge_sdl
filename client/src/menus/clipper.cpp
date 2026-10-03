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

std::vector<Vertex> Clipper::clipLeft(const std::vector<Vertex>& vertices)
{
   std::vector<Vertex> result;
   if (vertices.empty())
   {
      return result;
   }

   size_t previous = vertices.size() - 1;
   int c1 = getClipFlags(vertices[previous]) & Left;

   for (size_t i = 0; i < vertices.size(); i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Left;
      const Vertex& v1 = vertices[previous];
      const Vertex& v2 = vertices[i];

      if (!c1)
      {
         result.push_back(v1);
      }

      if (c1 != c2)
      {
         // v1.x + (v2.x - v1.x)*t = _left
         const float t = (_left - v1.x) / (v2.x - v1.x);
         result.push_back(v1 + (v2 - v1) * t);
      }

      previous = i;
      c1 = c2;
   }

   return result;
}

std::vector<Vertex> Clipper::clipRight(const std::vector<Vertex>& vertices)
{
   std::vector<Vertex> result;
   if (vertices.empty())
   {
      return result;
   }

   size_t previous = vertices.size() - 1;
   int c1 = getClipFlags(vertices[previous]) & Right;

   for (size_t i = 0; i < vertices.size(); i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Right;
      const Vertex& v1 = vertices[previous];
      const Vertex& v2 = vertices[i];

      if (!c1)
      {
         result.push_back(v1);
      }

      if (c1 != c2)
      {
         // v1.x + (v2.x - v1.x)*t = _right
         const float t = (_right - v1.x) / (v2.x - v1.x);
         result.push_back(v1 + (v2 - v1) * t);
      }

      previous = i;
      c1 = c2;
   }

   return result;
}

std::vector<Vertex> Clipper::clipTop(const std::vector<Vertex>& vertices)
{
   std::vector<Vertex> result;
   if (vertices.empty())
   {
      return result;
   }

   size_t previous = vertices.size() - 1;
   int c1 = getClipFlags(vertices[previous]) & Top;

   for (size_t i = 0; i < vertices.size(); i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Top;
      const Vertex& v1 = vertices[previous];
      const Vertex& v2 = vertices[i];

      if (!c1)
      {
         result.push_back(v1);
      }

      if (c1 != c2)
      {
         // v1.y + (v2.y - v1.y)*t = _top
         const float t = (_top - v1.y) / (v2.y - v1.y);
         result.push_back(v1 + (v2 - v1) * t);
      }

      previous = i;
      c1 = c2;
   }

   return result;
}

std::vector<Vertex> Clipper::clipBottom(const std::vector<Vertex>& vertices)
{
   std::vector<Vertex> result;
   if (vertices.empty())
   {
      return result;
   }

   size_t previous = vertices.size() - 1;
   int c1 = getClipFlags(vertices[previous]) & Bottom;

   for (size_t i = 0; i < vertices.size(); i++)
   {
      const int c2 = getClipFlags(vertices[i]) & Bottom;
      const Vertex& v1 = vertices[previous];
      const Vertex& v2 = vertices[i];

      if (!c1)
      {
         result.push_back(v1);
      }

      if (c1 != c2)
      {
         // v1.y + (v2.y - v1.y)*t = _bottom
         const float t = (_bottom - v1.y) / (v2.y - v1.y);
         result.push_back(v1 + (v2 - v1) * t);
      }

      previous = i;
      c1 = c2;
   }

   return result;
}

std::vector<Vertex> Clipper::clip(const std::vector<Vertex>& vertices)
{
   std::vector<Vertex> left = clipLeft(vertices);
   std::vector<Vertex> right = clipRight(left);
   std::vector<Vertex> top = clipTop(right);
   return clipBottom(top);
}

bool Clipper::visible(const std::vector<Vertex>& vertices) const
{
   int flags = -1;
   for (const Vertex& vertex : vertices)
   {
      flags &= getClipFlags(vertex);
   }
   return flags == 0;
}

bool Clipper::enable(const std::vector<Vertex>& vertices)
{
   int all_flags = 0;
   int any_flags = Left | Right | Top | Bottom;
   for (const Vertex& vertex : vertices)
   {
      const int flag = getClipFlags(vertex);
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
   if (const auto frame_buffer = FrameBuffer::Instance())
   {
      width = frame_buffer->get().width();
      height = frame_buffer->get().height();
   }
   else
   {
      width = static_cast<int>(activeDevice().getWidth());
      height = static_cast<int>(activeDevice().getHeight());
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
      glScissor(0, 0, static_cast<int>(activeDevice().getWidth()), static_cast<int>(activeDevice().getHeight()));
   }
}
