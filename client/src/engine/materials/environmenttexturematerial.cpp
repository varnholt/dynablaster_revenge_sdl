#include "environmenttexturematerial.h"
#include "animation/motionmixer.h"
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

EnvironmentTextureMaterial::EnvironmentTextureMaterial() : Material(MAP_DIFFUSE | MAP_REFLECT)
{
}

EnvironmentTextureMaterial::EnvironmentTextureMaterial(
   const std::string& color_map,
   const std::string& environment_map,
   const std::string& specular_map
)
    : Material(MAP_DIFFUSE | MAP_REFLECT)
{
   addTexture(_color_map, color_map);
   addTexture(_diffuse_map, environment_map);
   addTexture(_specular_map, specular_map, 1 | 2 | 4);
}

void EnvironmentTextureMaterial::init()
{
   _shader = activeDevice().loadShader("environmenttexture-vert.glsl", "environmenttexture-frag.glsl");

   _param_specular = activeDevice().getParameterIndex("specularmap");
   _param_diffuse = activeDevice().getParameterIndex("diffusemap");
   _param_texture = activeDevice().getParameterIndex("texturemap");
   _param_camera = activeDevice().getParameterIndex("camera");
}

void EnvironmentTextureMaterial::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0).name());
   addTexture(_diffuse_map, "diffuse_level");
   addTexture(_specular_map, getTextureSlot(1).name(), 1 | 2 | 4);
}

void EnvironmentTextureMaterial::updateGeometry(const VertexBuffer& vertex_buffer, const Geometry& geometry)
{
   const std::span<const Vector> vertices = geometry.getVertices();
   const std::span<const Vector> normals = geometry.getNormals();
   const std::span<const UV> uv = geometry.getUV(1);

   const std::span<Vertex> destination = activeDevice().lockVertexBuffer<Vertex>(vertex_buffer.getVertexBuffer());
   for (int32_t i = 0; i < geometry.getVertexCount(); i++)
   {
      destination[i].position.x = vertices[i].x;
      destination[i].position.y = vertices[i].y;
      destination[i].position.z = vertices[i].z;

      destination[i].normal.x = normals[i].x;
      destination[i].normal.y = normals[i].y;
      destination[i].normal.z = normals[i].z;

      destination[i].uv.u = uv[i].u;
      destination[i].uv.v = uv[i].v;
   }
   activeDevice().unlockVertexBuffer(vertex_buffer.getVertexBuffer());
}

void EnvironmentTextureMaterial::addGeometry(Geometry& geometry)
{
   std::optional<std::reference_wrapper<VertexBuffer>> pooled = _pool->get(geometry);
   if (!pooled)
   {
      VertexBuffer& vertex_buffer = _pool->add(geometry);
      pooled = vertex_buffer;

      const bool dynamic = geometry.getParent().hasSkinning();
      activeDevice().allocateVertexBuffer(vertex_buffer.getVertexBuffer(), sizeof(Vertex) * geometry.getVertexCount(), dynamic);
      updateGeometry(vertex_buffer, geometry);

      activeDevice().allocateIndexBuffer(vertex_buffer.getIndexBuffer(), geometry.getIndexCount() * sizeof(uint16_t));
      std::ranges::copy(geometry.getIndices(), activeDevice().lockIndexBuffer<uint16_t>(vertex_buffer.getIndexBuffer()).begin());
      activeDevice().unlockIndexBuffer(vertex_buffer.getIndexBuffer());

      vertex_buffer.setIndexCount(geometry.getIndexCount());
   }

   _buffers.push_back({geometry, *pooled});
}

void EnvironmentTextureMaterial::begin()
{
   Material::begin();

   glBindTexture(GL_TEXTURE_2D, _specular_map);

   glActiveTexture(GL_TEXTURE1_ARB);
   glBindTexture(GL_TEXTURE_2D, _diffuse_map);

   glActiveTexture(GL_TEXTURE2_ARB);
   glBindTexture(GL_TEXTURE_2D, _color_map);

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_param_specular, 0);
   activeDevice().bindSampler(_param_diffuse, 1);
   activeDevice().bindSampler(_param_texture, 2);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
}

void EnvironmentTextureMaterial::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);

   glActiveTexture(GL_TEXTURE0);

   activeDevice().setShader(0);
}

void EnvironmentTextureMaterial::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      const VertexBuffer& vertex_buffer = buffer.vertex_buffer;
      Geometry& geometry = buffer.geometry;

      if (geometry.isVisible())
      {
         const Matrix inverse_view = (geometry.getTransform() * _camera).invert();
         const Vector object_space_camera = inverse_view.translation();
         activeDevice().setParameter(_param_camera, object_space_camera);

         activeDevice().push(geometry.getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));
         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer.getIndexBuffer());
         glDrawElements(GL_TRIANGLES, vertex_buffer.getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

         activeDevice().pop();
      }
   }

   end();
}

void EnvironmentTextureMaterial::update(float /*frame*/, const Matrix& camera)
{
   _camera = camera;
}
