// outline rendering implementation

#include "outlining.h"
#include "gldevice.h"
#include "nodes/mesh.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "renderdevice.h"
#include "tools/stream.h"

Outlining::Outlining() : Material(-4)
{
}

void Outlining::init()
{
   _shader = activeDevice().loadShader("outlining-vert.glsl", "outlining-frag.glsl");
   _param_color = activeDevice().getParameterIndex("u_color");
}

void Outlining::load(Stream&)
{
}

void Outlining::add(Geometry& geometry)
{
   VertexBuffer& vertex_buffer = _pool->add(geometry);

   activeDevice().allocateVertexBuffer(vertex_buffer.getVertexBuffer(), sizeof(Vector) * geometry.getVertexCount());
   const std::span<Vector> destination = activeDevice().lockVertexBuffer<Vector>(vertex_buffer.getVertexBuffer());
   const std::span<const Vector> source = geometry.getVertices();
   for (int32_t i = 0; i < geometry.getVertexCount(); i++)
   {
      destination[i].x = source[i].x;
      destination[i].y = source[i].y;
      destination[i].z = source[i].z;
   }
   activeDevice().unlockVertexBuffer(vertex_buffer.getVertexBuffer());

   activeDevice().allocateIndexBuffer(vertex_buffer.getIndexBuffer(), geometry.getEdgeCount() * 2 * sizeof(uint16_t), true);

   _buffers.push_back({geometry, vertex_buffer});
}

int32_t Outlining::calcEdgeIndices(uint32_t index_buffer, const Geometry& geometry, const Vector& viewer)
{
   int32_t num = 0;

   const std::span<uint16_t> indices = activeDevice().lockIndexBuffer<uint16_t>(index_buffer);
   const std::span<const Vector> vertices = geometry.getVertices();
   for (const Edge& edge : geometry.getEdges())
   {
      // vertices of two triangles with shared edge v1->v2
      const Vector& v1 = vertices[edge.i1];
      const Vector& v2 = vertices[edge.i2];
      const Vector& v3 = vertices[edge.i3];
      const Vector& v4 = vertices[edge.i4];

      // takes 8.5MI for 90.000 edges
      const float direction1 = ((v2.y - v1.y) * (v3.z - v1.z) - (v2.z - v1.z) * (v3.y - v1.y)) * (v1.x - viewer.x) +
                               ((v2.z - v1.z) * (v3.x - v1.x) - (v2.x - v1.x) * (v3.z - v1.z)) * (v1.y - viewer.y) +
                               ((v2.x - v1.x) * (v3.y - v1.y) - (v2.y - v1.y) * (v3.x - v1.x)) * (v1.z - viewer.z);

      const float direction2 = ((v2.y - v1.y) * (v1.z - v4.z) - (v2.z - v1.z) * (v1.y - v4.y)) * (v1.x - viewer.x) +
                               ((v2.z - v1.z) * (v1.x - v4.x) - (v2.x - v1.x) * (v1.z - v4.z)) * (v1.y - viewer.y) +
                               ((v2.x - v1.x) * (v1.y - v4.y) - (v2.y - v1.y) * (v1.x - v4.x)) * (v1.z - viewer.z);

      // draw edge if:
      // - one poly facing to viewer, one face away
      // - both polys facing to viewer and normals discontinue (precalc'd in flags)
      if ((direction1 * direction2) < 0 || (direction1 < 0 && direction2 < 0 && edge.flags))
      {
         indices[num] = edge.i1;
         indices[num + 1] = edge.i2;
         num += 2;
      }
   }

   activeDevice().unlockIndexBuffer(index_buffer);

   return num;
}

void Outlining::update(float, const Matrix&)
{
   for (const Buffer& buffer : _buffers)
   {
      // inverse-transform camera position to local object space
      const Matrix inverse_view = (buffer.geometry.get().getTransform() * _camera).invert();
      const Vector viewer = inverse_view.translation();

      const int32_t count = calcEdgeIndices(buffer.vertex_buffer.get().getIndexBuffer(), buffer.geometry, viewer);
      buffer.vertex_buffer.get().setIndexCount(count);
   }
}

void Outlining::begin()
{
   Material::begin();

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data

   activeDevice().setShader(_shader);  // just transform from object to camera

   // pass2: render outlines
   activeDevice().setParameter(_param_color, Vector4(0.0f, 0.0f, 0.0f, 1.0f));  // black
   glEnable(GL_BLEND);

   glDepthMask(GL_FALSE);
}

void Outlining::end()
{
   glDepthMask(GL_TRUE);

   // disable vertex array
   glDisableVertexAttribArray(0);

   glDisable(GL_BLEND);

   activeDevice().setShader(0);
}

void Outlining::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      // set transformation
      const VertexBuffer& vertex_buffer = buffer.vertex_buffer;
      activeDevice().push(buffer.geometry.get().getTransform());

      glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.getVertexBuffer());
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer.getIndexBuffer());
      glDrawElements(GL_LINES, vertex_buffer.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

      activeDevice().pop();
   }

   end();
}
