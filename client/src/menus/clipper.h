#pragma once

#include "vertex.h"

#include <vector>

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

   std::vector<Vertex> clip(const std::vector<Vertex>& vertices);
   void enable();
   bool enable(const std::vector<Vertex>& source_bounding);
   void disable();
   bool visible(const std::vector<Vertex>& vertices) const;
   float getWidth() const;
   float getHeight() const;

private:
   int getClipFlags(const Vertex& vertex) const;
   std::vector<Vertex> clipRight(const std::vector<Vertex>& vertices);
   std::vector<Vertex> clipLeft(const std::vector<Vertex>& vertices);
   std::vector<Vertex> clipTop(const std::vector<Vertex>& vertices);
   std::vector<Vertex> clipBottom(const std::vector<Vertex>& vertices);

   float _left;
   float _top;
   float _right;
   float _bottom;
   float _screen_width = 1920.0f;
   float _screen_height = 1080.0f;
   bool _enabled = false;
};
