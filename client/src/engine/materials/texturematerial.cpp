#include "texturematerial.h"
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/geometry.h"
#include "render/renderbuffer.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

TextureMaterial::TextureMaterial(SceneGraph* scene) : Material(scene, MAP_DIFFUSE)
{
}

TextureMaterial::TextureMaterial(SceneGraph* scene, const char* texture_map) : Material(scene, MAP_DIFFUSE)
{
   addTexture(_color_map, texture_map);
}

void TextureMaterial::load(Stream* stream)
{
   Material::load(stream);

   if (getTextureSlot(0))
   {
      addTexture(_color_map, getTextureSlot(0)->name());
   }
}

void TextureMaterial::init()
{
   _shader = activeDevice->loadShader("texturemapping-vert.glsl", "texturemapping-frag.glsl");
   _param_texture = activeDevice->getParameterIndex("texturemap");
}

void TextureMaterial::addGeometry(Geometry* geometry)
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
         if (uv)
         {
            destination[i].uv.u = uv[i].u;
            destination[i].uv.v = uv[i].v;
         }
      }
      activeDevice->unlockVertexBuffer(vertex_buffer->getVertexBuffer());

      vertex_buffer->setIndexBuffer(geometry->getIndices(), geometry->getIndexCount());
   }

   _buffers.push_back({geometry, vertex_buffer});
}

void TextureMaterial::begin()
{
   Material::begin();

   activeDevice->setShader(_shader);

   glBindTexture(GL_TEXTURE_2D, _color_map);
   activeDevice->bindSampler(_param_texture, 0);

   // enable required vertex attributes (0=position, 1=normal, 2=texcoord)
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
}

void TextureMaterial::end()
{
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);

   activeDevice->setShader(0);
}

void TextureMaterial::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      VertexBuffer* vertex_buffer = buffer.vertex_buffer;
      Geometry* geometry = buffer.geometry;

      if (geometry->isVisible())
      {
         if (!geometry->getBoneCount())
         {
            activeDevice->push(geometry->getTransform());
         }

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));
         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer->getIndexBuffer());
         glDrawElements(GL_TRIANGLES, vertex_buffer->getIndexCount(), GL_UNSIGNED_SHORT, nullptr);  // render

         if (!geometry->getBoneCount())
         {
            activeDevice->pop();
         }
      }
   }

   end();
}

void TextureMaterial::update(float /*frame*/, Node** /*node_list*/, const Matrix&)
{
}
