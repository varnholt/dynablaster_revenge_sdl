#pragma once

#include "render/texture.h"
#include "math/matrix.h"
#include "tools/array.h"
#include "vertex.h"

#include <cstdint>

/// \brief signed-distance-field bitmap font; tinted via setColor() and a uniform on the shared
/// fontoutlines shader.
class BitmapFont
{
public:
   struct Parameter
   {
      int16_t c;
      uint16_t x;
      uint16_t y;
      uint16_t width;
      uint16_t height;
      int16_t basecolumn;
      int16_t baseline;
      uint16_t space;
   };

   BitmapFont(
      const char* filename,
      Parameter* description,
      float size = 1.0f,
      float spacing = 0.0f,
      float distance_radius = 0.0f,
      float outline_red = 0.0f,
      float outline_green = 0.0f,
      float outline_blue = 0.0f,
      float outline_alpha = 1.0f,
      float outline_radius = 0.15f,
      float soft_radius = 0.06f,
      float thickness = 0.0f
   );

   bool isCharAvailable(char c) const;
   void setOutlineColor(float r, float g, float b, float a);
   void getOutlineColor(float& r, float& g, float& b, float& a);
   void setColor(float r, float g, float b, float a);
   float buildVertices(float size, const char* text, float x, float y, float center_width = -1.0f, float center_height = -1.0f);
   const Array<Vertex>& getVertices() const;
   void draw(const Array<Vertex>& vertices, const Matrix& transform = Matrix());
   void draw();
   uint32_t getTexture();

   //! getter cursor dimensions
   void getCursor(float scale, int cursor_position, float& left, float& right, float& top, float& bottom);

   Parameter* getCharParameter(char c) const;

private:
   uint32_t _shader = 0;
   Texture _texture;
   int _param_texture = 0;
   int _param_outline_color = 0;
   int _param_color = 0;
   int _param_soft_radius = 0;
   int _param_outline_radius = 0;
   int _param_thickness = 0;
   int _param_sample_offset = 0;

   // non-owning, points into a static glyph table
   Parameter* _description;
   float _size;
   float _spacing;
   float _radius;
   float _scale_u = 0.0f;
   float _scale_v = 0.0f;
   float _outline_red;
   float _outline_green;
   float _outline_blue;
   float _outline_alpha;
   float _outline_radius;
   float _soft_radius;
   float _thickness;

   float _color_red = 1.0f;
   float _color_green = 1.0f;
   float _color_blue = 1.0f;
   float _color_alpha = 1.0f;

   float _baseline = 0.0f;
   float _base_column = 0.0f;
   mutable Array<Vertex> _vertices;

   // dynamically re-uploaded each draw() call (text changes every frame) - lazily created
   uint32_t _vertex_buffer = 0;
};
