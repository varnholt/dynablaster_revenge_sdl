#include "extramapping.h"
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/geometry.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

ExtraMapping::ExtraMapping() : Material(-3)
{
}

ExtraMapping::ExtraMapping(const std::string& map) : Material(-3)
{
   addTexture(_color_map, map);
}

void ExtraMapping::init()
{
   _shader = activeDevice().loadShader("texturemapping-vert.glsl", "texturemapping-frag.glsl");

   _param_texture = activeDevice().getParameterIndex("texturemap");
}

void ExtraMapping::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0).name());
}

void ExtraMapping::addGeometry(Geometry& geometry)
{
   std::optional<std::reference_wrapper<VertexBuffer>> pooled = _pool->get(geometry);
   if (!pooled)
   {
      VertexBuffer& vertex_buffer = _pool->add(geometry);
      pooled = vertex_buffer;

      const std::span<const Vector> vertices = geometry.getVertices();
      const std::span<const UV> uv = geometry.getUV(1);

      activeDevice().allocateVertexBuffer(vertex_buffer.getVertexBuffer(), sizeof(Vertex) * geometry.getVertexCount());
      const std::span<Vertex> destination = activeDevice().lockVertexBuffer<Vertex>(vertex_buffer.getVertexBuffer());
      for (int32_t i = 0; i < geometry.getVertexCount(); i++)
      {
         destination[i].position.x = vertices[i].x;
         destination[i].position.y = vertices[i].y;
         destination[i].position.z = vertices[i].z;
         destination[i].uv.u = uv[i].u;
         destination[i].uv.v = uv[i].v;
      }
      activeDevice().unlockVertexBuffer(vertex_buffer.getVertexBuffer());

      vertex_buffer.setIndexBuffer(geometry.getIndices());
   }

   _buffers.push_back({geometry, *pooled});
}

void ExtraMapping::update(float, const Matrix& camera)
{
   _camera = camera;
}

void ExtraMapping::begin()
{
   Material::begin();

   glEnable(GL_BLEND);
   glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

   glBindTexture(GL_TEXTURE_2D, _color_map);

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_param_texture, 0);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(2);  // texcoord
}

void ExtraMapping::end()
{
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(2);  // texcoord

   glDisable(GL_BLEND);

   activeDevice().setShader(0);
}

void ExtraMapping::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      const VertexBuffer& vertex_buffer = buffer.vertex_buffer;
      Geometry& geometry = buffer.geometry;

      if (geometry.isVisible())
      {
         activeDevice().push(geometry.getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer.getIndexBuffer());
         glDrawElements(GL_TRIANGLES, vertex_buffer.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

         activeDevice().pop();
      }
   }

   end();
}
