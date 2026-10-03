#include "bitmapfont.h"
#include "defaultshader.h"
#include "framework/gldevice.h"
#include "image/image.h"
#include "image/imagepool.h"
#include "math/matrix.h"
#include "math/vector4.h"
#include "render/texturepool.h"

#include <algorithm>
#include <array>
#include <vector>

BitmapFont::BitmapFont(
   const char* filename,
   Parameter* description,
   float size,
   float spacing,
   float distance_radius,
   float outline_red,
   float outline_green,
   float outline_blue,
   float outline_alpha,
   float outline_radius,
   float soft_radius,
   float thickness
)
    : _description(description),
      _size(size),
      _spacing(spacing),
      _radius(distance_radius),
      _outline_red(outline_red),
      _outline_green(outline_green),
      _outline_blue(outline_blue),
      _outline_alpha(outline_alpha),
      _outline_radius(outline_radius),
      _soft_radius(soft_radius),
      _thickness(thickness)
{
   _shader = activeDevice->loadShader("fontoutlines-vert.glsl", "fontoutlines-frag.glsl");
   _param_texture = activeDevice->getParameterIndex("distanceMap");
   _param_outline_color = activeDevice->getParameterIndex("outlineColor");
   _param_color = activeDevice->getParameterIndex("color");

   _param_soft_radius = activeDevice->getParameterIndex("aaRadius");
   _param_outline_radius = activeDevice->getParameterIndex("outlineRadius");
   _param_thickness = activeDevice->getParameterIndex("threshold");
   _param_sample_offset = activeDevice->getParameterIndex("sampleOffset");

   const Image& image = ImagePool::Instance().getImage(filename);
   _texture = TexturePool::Instance().getTexture(image, TexturePool::Linear | TexturePool::Clamp);
   _scale_u = 1.0f / image.getWidth();
   _scale_v = 1.0f / image.getHeight();
}

bool BitmapFont::isCharAvailable(char c) const
{
   return getCharParameter(c) != nullptr;
}

void BitmapFont::setOutlineColor(float r, float g, float b, float a)
{
   _outline_red = r;
   _outline_green = g;
   _outline_blue = b;
   _outline_alpha = a;
}

void BitmapFont::getOutlineColor(float& r, float& g, float& b, float& a)
{
   r = _outline_red;
   g = _outline_green;
   b = _outline_blue;
   a = _outline_alpha;
}

void BitmapFont::setColor(float r, float g, float b, float a)
{
   _color_red = r;
   _color_green = g;
   _color_blue = b;
   _color_alpha = a;
}

BitmapFont::Parameter* BitmapFont::getCharParameter(char c) const
{
   // only 7-bit ascii has glyphs; char is unsigned on some platforms (ARM)
   const auto code = static_cast<uint8_t>(c);
   if (code < 128)
   {
      return &_description[code];
   }

   return nullptr;
}

float BitmapFont::buildVertices(float size, const char* text, float x, float y, float center_width, float center_height)
{
   _vertices.clear();

   // remember text position for cursor
   _base_column = x;
   _baseline = y;

   size *= _size;

   x -= _radius * size;

   if (center_width >= 0.0f)
   {
      float width = 0.0f;
      for (const char* character = text; *character; ++character)
      {
         if (const Parameter* param = getCharParameter(*character))
         {
            width += (param->space + _spacing);
         }
      }
      width -= _spacing;  // remove last spacing

      x += (center_width - (width + _radius * 2) * size) * 0.5f;
   }

   if (center_height >= 0.0f)
   {
      float height = center_height;
      if (const Parameter* param = getCharParameter('M'))
      {
         height = (param->height - _radius * 2) * size;
      }
      y -= (center_height - height) * 0.5f;
   }

   while (*text)
   {
      const char c = *text++;
      const Parameter* param = getCharParameter(c);

      const float x_left = x - (param->basecolumn) * size;
      const float x_right = x - (param->basecolumn - param->width) * size;
      const float y_top = y - (param->baseline) * size;
      const float y_bottom = y - (param->baseline + param->height) * size;

      _vertices.push_back(Vertex(x_left, y_top, param->x * _scale_u, (param->y + param->height) * _scale_v));
      _vertices.push_back(Vertex(x_right, y_top, (param->x + param->width) * _scale_u, (param->y + param->height) * _scale_v));
      _vertices.push_back(Vertex(x_right, y_bottom, (param->x + param->width) * _scale_u, param->y * _scale_v));
      _vertices.push_back(Vertex(x_left, y_bottom, param->x * _scale_u, param->y * _scale_v));

      x += (param->space + _spacing) * size;
   }

   return x;
}

const std::vector<Vertex>& BitmapFont::getVertices() const
{
   return _vertices;
}

void BitmapFont::draw()
{
   draw(_vertices);
}

void BitmapFont::draw(const std::vector<Vertex>& vertices, const Matrix& transform)
{
   const int quad_count = static_cast<int>(vertices.size()) / 4;
   if (quad_count <= 0)
   {
      return;
   }

   activeDevice->setShader(_shader);

   // Upload after binding the font shader, including any caller's page-space offset.
   activeDevice->push(transform);

   const float sample_offset = (vertices[1].u - vertices[0].u) / (vertices[1].x - vertices[0].x);

   glBindTexture(GL_TEXTURE_2D, _texture);
   activeDevice->bindSampler(_param_texture, 0);
   activeDevice->setParameter(_param_outline_color, Vector4(_outline_red, _outline_green, _outline_blue, _outline_alpha));
   activeDevice->setParameter(_param_color, Vector4(_color_red, _color_green, _color_blue, _color_alpha));
   activeDevice->setParameter(_param_soft_radius, _soft_radius);
   activeDevice->setParameter(_param_outline_radius, _outline_radius);
   activeDevice->setParameter(_param_thickness, _thickness);
   activeDevice->setParameter(_param_sample_offset, sample_offset * 1.0f);

   // GLES3 has no GL_QUADS - each 4-vertex quad becomes 2 triangles (0,1,2 / 0,2,3), rebuilt
   // into a plain interleaved (x,y,u,v) buffer every draw call
   constexpr std::array<int, 6> order = {0, 1, 2, 0, 2, 3};
   std::vector<float> data;
   data.reserve(static_cast<size_t>(quad_count) * order.size() * 4);
   for (int q = 0; q < quad_count; q++)
   {
      for (const int corner : order)
      {
         const Vertex& vertex = vertices[q * 4 + corner];
         data.push_back(vertex.x);
         data.push_back(vertex.y);
         data.push_back(vertex.u);
         data.push_back(vertex.v);
      }
   }

   const int size = static_cast<int>(data.size() * sizeof(float));
   if (_vertex_buffer == 0)
   {
      _vertex_buffer = activeDevice->createVertexBuffer(size, true);
   }
   else
   {
      activeDevice->allocateVertexBuffer(_vertex_buffer, size, true);
   }

   auto* destination = static_cast<float*>(activeDevice->lockVertexBuffer(_vertex_buffer, size));
   std::ranges::copy(data, destination);
   activeDevice->unlockVertexBuffer(_vertex_buffer);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, reinterpret_cast<GLvoid*>(sizeof(float) * 2));

   glDrawArrays(GL_TRIANGLES, 0, quad_count * 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);

   activeDevice->pop();

   // restore the shared menu shader rather than "no shader" - see defaultshader.h
   activeDevice->setShader(getDefaultMenuShader());
}

uint32_t BitmapFont::getTexture()
{
   return _texture;
}

void BitmapFont::getCursor(float size, int cursor_position, float& left, float& right, float& top, float& bottom)
{
   size *= _size;
   cursor_position *= 4;

   const int vertex_count = static_cast<int>(_vertices.size());
   if (vertex_count < 4)
   {
      // empty string
      const Parameter* param = getCharParameter('M');
      left = _base_column;
      right = left + param->space * size + _radius * size * 2;

      top = _base_column - (param->baseline) * size;
      bottom = _baseline - (param->baseline + param->height) * size;
   }
   else if (cursor_position >= vertex_count)
   {
      // cursor at end of text
      const Parameter* param = getCharParameter('M');
      cursor_position = vertex_count - 4;

      left = _vertices[cursor_position + 1].x - (_radius * size * 2) + _spacing * size;
      right = left + param->space * size + _radius * size * 2;
   }
   else
   {
      left = _vertices[cursor_position].x;
      right = _vertices[cursor_position + 1].x;
   }

   const Parameter* param = getCharParameter('M');
   top = _baseline - (param->baseline) * size;
   bottom = _baseline - (param->baseline + param->height) * size;

   top -= _radius * size;
   bottom += _radius * size;

   left += _radius * size;
   right -= _radius * size;
}
