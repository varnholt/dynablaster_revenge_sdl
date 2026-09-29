#pragma once

#include "tools/array.h"
#include "vertex.h"

class Clipper
{
public:
   enum Flags
   {
      Left = 1,
      Right = 2,
      Top = 4,
      Bottom = 8
   };

   Clipper(float left, float top, float right, float bottom);

   void setBounds(float left, float top, float right, float bottom);

   Array<Vertex> clip(const Array<Vertex>& vertices);
   void enable();
   bool enable(const Array<Vertex>& source_bounding);
   void disable();
   bool visible(const Array<Vertex>& vertices) const;
   float getWidth() const;
   float getHeight() const;

private:
   int getClipFlags(const Vertex& vertex) const;
   Array<Vertex> clipRight(const Array<Vertex>& vertices);
   Array<Vertex> clipLeft(const Array<Vertex>& vertices);
   Array<Vertex> clipTop(const Array<Vertex>& vertices);
   Array<Vertex> clipBottom(const Array<Vertex>& vertices);

   float _left;
   float _top;
   float _right;
   float _bottom;
   float _screen_width = 1920.0f;
   float _screen_height = 1080.0f;
   bool _enabled = false;
};
