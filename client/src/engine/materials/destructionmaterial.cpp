#include "destructionmaterial.h"
#include <cmath>
#include "animation/motionmixer.h"
#include "framework/gldevice.h"
#include "image/image.h"
#include "nodes/camera.h"
#include "nodes/mesh.h"
#include "render/renderbuffer.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

DestructionMaterial::DestructionMaterial(SceneGraph* scene) : Material(scene, MAP_DIFFUSE | MAP_REFLECT)
{
}

DestructionMaterial::DestructionMaterial(
   SceneGraph* scene,
   const char* color_map,
   const char* environment_map,
   const char* specular_map,
   const char* shadow_map,
   Camera* shadow_camera
)
    : Material(scene, MAP_DIFFUSE | MAP_REFLECT), _shadow_camera(shadow_camera)
{
   addTexture(_color_map, color_map);
   addTexture(_diffuse_map, environment_map);
   addTexture(_specular_map, specular_map);
   addTexture(_shadow_map, shadow_map);
}

void DestructionMaterial::init()
{
   _shader = activeDevice->loadShader("destruction-vert.glsl", "destruction-frag.glsl");

   _param_specular = activeDevice->getParameterIndex("specularmap");
   _param_diffuse = activeDevice->getParameterIndex("diffusemap");
   _param_texture = activeDevice->getParameterIndex("texturemap");
   _param_shadow = activeDevice->getParameterIndex("shadowmap");
   _param_shadow_camera = activeDevice->getParameterIndex("shadowCamera");
}

void DestructionMaterial::load(Stream* stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0)->name());
   addTexture(_diffuse_map, "diffuse_level");
   addTexture(_specular_map, getTextureSlot(1)->name());
}

void DestructionMaterial::addGeometry(Geometry* geometry)
{
   VertexBuffer* vertex_buffer = _pool->get(geometry);
   if (!vertex_buffer)
   {
      vertex_buffer = _pool->add(geometry);

      const Vector* vertices = geometry->getVertices();
      const Vector* normals = geometry->getNormals();
      const UV* uv = geometry->getUV(1);

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

         destination[i].uv.u = uv[i].u;
         destination[i].uv.v = uv[i].v;
      }
      activeDevice->unlockVertexBuffer(vertex_buffer->getVertexBuffer());

      vertex_buffer->setIndexBuffer(geometry->getIndices(), geometry->getIndexCount());
   }

   _buffers.push_back({geometry, vertex_buffer});
}

void DestructionMaterial::begin()
{
   Material::begin();

   glBindTexture(GL_TEXTURE_2D, _specular_map);

   glActiveTexture(GL_TEXTURE1_ARB);
   glBindTexture(GL_TEXTURE_2D, _diffuse_map);

   glActiveTexture(GL_TEXTURE2_ARB);
   glBindTexture(GL_TEXTURE_2D, _color_map);

   glActiveTexture(GL_TEXTURE3_ARB);
   glBindTexture(GL_TEXTURE_2D, _shadow_map);

   activeDevice->setShader(_shader);
   activeDevice->bindSampler(_param_specular, 0);
   activeDevice->bindSampler(_param_diffuse, 1);
   activeDevice->bindSampler(_param_texture, 2);
   activeDevice->bindSampler(_param_shadow, 3);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);

   Matrix camera;
   if (_shadow_camera)
   {
      GLDevice* device = static_cast<GLDevice*>(activeDevice);
      device->pushProjection();

      camera = _shadow_camera->getTransform().getView();
      float fov = _shadow_camera->getFOV();
      fov = std::tan(fov * 0.5) * 0.75;
      const float z_near = _shadow_camera->getNear();
      const float z_far = _shadow_camera->getFar();

      activeDevice->setCamera(camera, fov, z_near, z_far, _shadow_camera->getPerspectiveMode());

      camera = device->getProjectionMatrix();
      camera.normalizeZ();
      device->popProjection();
   }
   activeDevice->setParameter(_param_shadow_camera, camera);
}

void DestructionMaterial::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);

   glActiveTexture(GL_TEXTURE3_ARB);

   glActiveTexture(GL_TEXTURE0);

   activeDevice->setShader(0);
}

void DestructionMaterial::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      VertexBuffer* vertex_buffer = buffer.vertex_buffer;
      Geometry* geometry = buffer.geometry;

      if (geometry->isVisible())
      {
         activeDevice->push(geometry->getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));
         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer->getIndexBuffer());
         glDrawElements(GL_TRIANGLES, vertex_buffer->getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

         activeDevice->pop();
      }
   }

   end();
}

void DestructionMaterial::update(float /*frame*/, Node** /*node_list*/, const Matrix&)
{
}
