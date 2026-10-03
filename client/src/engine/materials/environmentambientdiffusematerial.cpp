#include "environmentambientdiffusematerial.h"
#include "animation/motionmixer.h"
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

EnvironmentAmbientDiffuseMaterial::EnvironmentAmbientDiffuseMaterial() : Material(MAP_AMBIENT | MAP_DIFFUSE | MAP_REFLECT)
{
}

EnvironmentAmbientDiffuseMaterial::EnvironmentAmbientDiffuseMaterial(
   const std::string& ambient_map,
   const std::string& diffuse_map,
   const std::string& specular_map
)
    : Material(MAP_AMBIENT | MAP_DIFFUSE | MAP_REFLECT)
{
   addTexture(_ambient_map, ambient_map);
   addTexture(_diffuse_map, diffuse_map);
   addTexture(_specular_map, specular_map);
}

void EnvironmentAmbientDiffuseMaterial::init()
{
   _shader = activeDevice().loadShader("environmentambientdiffuse-vert.glsl", "environmentambientdiffuse-frag.glsl");

   _param_specular = activeDevice().getParameterIndex("specularmap");
   _param_ambient = activeDevice().getParameterIndex("ambientmap");
   _param_diffuse = activeDevice().getParameterIndex("diffusemap");
   _param_camera = activeDevice().getParameterIndex("camera");
}

void EnvironmentAmbientDiffuseMaterial::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_ambient_map, getTextureSlot(0).name());
   addTexture(_diffuse_map, getTextureSlot(1).name());
   addTexture(_specular_map, getTextureSlot(2).name());
}

void EnvironmentAmbientDiffuseMaterial::addGeometry(Geometry& geometry)
{
   std::optional<std::reference_wrapper<VertexBuffer>> pooled = _pool->get(geometry);
   if (!pooled)
   {
      VertexBuffer& vertex_buffer = _pool->add(geometry);
      pooled = vertex_buffer;

      const std::span<const Vector> vertices = geometry.getVertices();
      const std::span<const Vector> normals = geometry.getNormals();
      const std::span<const UV> uv = geometry.getUV(1);

      activeDevice().allocateVertexBuffer(vertex_buffer.getVertexBuffer(), sizeof(Vertex) * geometry.getVertexCount());
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

      vertex_buffer.setIndexBuffer(geometry.getIndices());
   }

   _buffers.push_back({geometry, *pooled});
}

void EnvironmentAmbientDiffuseMaterial::begin()
{
   Material::begin();

   glBindTexture(GL_TEXTURE_2D, _ambient_map);

   glActiveTexture(GL_TEXTURE1_ARB);
   glBindTexture(GL_TEXTURE_2D, _diffuse_map);

   glActiveTexture(GL_TEXTURE2_ARB);
   glBindTexture(GL_TEXTURE_2D, _specular_map);

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_param_ambient, 0);
   activeDevice().bindSampler(_param_diffuse, 1);
   activeDevice().bindSampler(_param_specular, 2);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
}

void EnvironmentAmbientDiffuseMaterial::end()
{
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);

   glActiveTexture(GL_TEXTURE0);

   activeDevice().setShader(0);
}

void EnvironmentAmbientDiffuseMaterial::renderDiffuse()
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

void EnvironmentAmbientDiffuseMaterial::update(float /*frame*/, const Matrix& camera)
{
   _camera = camera;
}
