#include "geometryvbo.h"

// engine
#include "gldevice.h"
#include "math/vector.h"
#include "render/geometry.h"
#include "render/uv.h"

#include <algorithm>
#include <cstddef>
#include <span>

GeometryVbo::GeometryVbo(const Geometry& geometry) : _geometry(geometry)
{
}

// TODO: the vertex and index buffers are never deleted
void GeometryVbo::initialize()
{
   // create vertex and index buffer
   _vertex_buffer = activeDevice().createVertexBuffer(_geometry.getVertexCount() * sizeof(Vertex3D));
   _index_buffer = activeDevice().createIndexBuffer(_geometry.getIndexCount() * sizeof(uint16_t));

   const std::span<const Vector> vertices = _geometry.getVertices();
   const std::span<const Vector> normals = _geometry.getNormals();
   const std::span<const UV> uvs = _geometry.getUV(1);

   // fill vertex buffer
   const std::span<Vertex3D> vertex = activeDevice().lockVertexBuffer<Vertex3D>(_vertex_buffer);
   for (int32_t i = 0; i < _geometry.getVertexCount(); i++)
   {
      vertex[i].position = vertices[i];
      vertex[i].normal = normals[i];
      vertex[i].u = uvs[i].u;
      vertex[i].v = uvs[i].v;
      vertex[i].index = 0.0f;
   }
   activeDevice().unlockVertexBuffer(_vertex_buffer);

   // fill index buffer
   std::ranges::copy(_geometry.getIndices(), activeDevice().lockIndexBuffer<uint16_t>(_index_buffer).begin());
   activeDevice().unlockIndexBuffer(_index_buffer);
}

void GeometryVbo::drawGeometry()
{
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), nullptr);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<const void*>(offsetof(Vertex3D, normal)));
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<const void*>(offsetof(Vertex3D, u)));

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
   glDrawElements(GL_TRIANGLES, _geometry.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);

   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
}
