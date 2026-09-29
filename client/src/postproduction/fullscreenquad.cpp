#include "fullscreenquad.h"
#include "gldevice.h"

#include <array>
#include <cstdint>

namespace
{
constexpr GLsizei vertex_stride = sizeof(float) * 5;
const auto* const texcoord_offset = reinterpret_cast<const GLvoid*>(sizeof(float) * 3);
}  // namespace

FullScreenQuad::FullScreenQuad()
{
   static constexpr std::array<float, 30> unit_quad = {
      -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f, -1.0f, 1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 1.0f,
   };

   glGenBuffers(1, &_unit_buffer);
   glBindBuffer(GL_ARRAY_BUFFER, _unit_buffer);
   glBufferData(GL_ARRAY_BUFFER, unit_quad.size() * sizeof(float), unit_quad.data(), GL_STATIC_DRAW);

   glGenBuffers(1, &_dynamic_buffer);
}

void FullScreenQuad::drawUnit()
{
   glBindBuffer(GL_ARRAY_BUFFER, _unit_buffer);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertex_stride, texcoord_offset);

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}

void FullScreenQuad::drawRect(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1)
{
   const std::array<float, 30> vertices = {
      x0, y0, 0.0f, u0, v0, x1, y0, 0.0f, u1, v0, x1, y1, 0.0f, u1, v1, x0, y0, 0.0f, u0, v0, x1, y1, 0.0f, u1, v1, x0, y1, 0.0f, u0, v1,
   };

   glBindBuffer(GL_ARRAY_BUFFER, _dynamic_buffer);
   glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_stride, nullptr);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, vertex_stride, texcoord_offset);

   glDrawArrays(GL_TRIANGLES, 0, 6);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
}
