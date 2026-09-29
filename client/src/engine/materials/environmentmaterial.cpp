#include "environmentmaterial.h"
#include "animation/motionmixer.h"
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/renderbuffer.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

EnvironmentMaterial::EnvironmentMaterial(SceneGraph* scene) : Material(scene, MAP_REFLECT)
{
}

EnvironmentMaterial::EnvironmentMaterial(SceneGraph* scene, const char* specular_map) : Material(scene, MAP_DIFFUSE | MAP_REFLECT)
{
   addTexture(_specular_map, specular_map, 1 | 2 | 4);
}

void EnvironmentMaterial::init()
{
   _shader = activeDevice->loadShader("environment-vert.glsl", "environment-frag.glsl");
   _param_specular = activeDevice->getParameterIndex("specularmap");
   _param_camera = activeDevice->getParameterIndex("camera");
}

void EnvironmentMaterial::load(Stream* stream)
{
   Material::load(stream);
   addTexture(_specular_map, getTextureSlot(0)->name(), 1 | 2 | 4);
}

void EnvironmentMaterial::addGeometry(Geometry* geometry)
{
   VertexBuffer* vertex_buffer = _pool->get(geometry);
   if (!vertex_buffer)
   {
      vertex_buffer = _pool->add(geometry);

      const Vector* vertices = geometry->getVertices();
      const Vector* normals = geometry->getNormals();

      activeDevice->allocateVertexBuffer(vertex_buffer->getVertexBuffer(), sizeof(Vertex) * geometry->getVertexCount());
      volatile Vertex* destination = static_cast<Vertex*>(activeDevice->lockVertexBuffer(vertex_buffer->getVertexBuffer()));
      for (int32_t i = 0; i < geometry->getVertexCount(); i++)
      {
         destination[i].position.x = vertices[i].x;
         destination[i].position.y = vertices[i].y;
         destination[i].position.z = vertices[i].z;
         destination[i].normal.x = normals[i].x;
         destination[i].normal.y = normals[i].y;
         destination[i].normal.z = normals[i].z;
      }
      activeDevice->unlockVertexBuffer(vertex_buffer->getVertexBuffer());

      vertex_buffer->setIndexBuffer(geometry->getIndices(), geometry->getIndexCount());
   }

   _buffers.push_back({geometry, vertex_buffer});
}

void EnvironmentMaterial::begin()
{
   Material::begin();

   glBindTexture(GL_TEXTURE_2D, _specular_map);

   activeDevice->setShader(_shader);
   activeDevice->bindSampler(_param_specular, 0);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);
}

void EnvironmentMaterial::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(1);

   activeDevice->setShader(0);
}

void EnvironmentMaterial::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      VertexBuffer* vertex_buffer = buffer.vertex_buffer;
      Geometry* geometry = buffer.geometry;

      if (geometry->isVisible())
      {
         const Matrix inverse_view = (geometry->getTransform() * _camera).invert();
         const Vector object_space_camera = inverse_view.translation();
         activeDevice->setParameter(_param_camera, object_space_camera);

         activeDevice->push(geometry->getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer->getIndexBuffer());
         glDrawElements(GL_TRIANGLES, vertex_buffer->getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

         activeDevice->pop();
      }
   }

   end();
}

void EnvironmentMaterial::update(float /*frame*/, Node** /*node_list*/, const Matrix& camera)
{
   _camera = camera;
}
