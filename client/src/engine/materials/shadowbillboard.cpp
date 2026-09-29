#include "shadowbillboard.h"
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

ShadowBillboard::ShadowBillboard(SceneGraph* scene) : Material(scene, -1)
{
}

ShadowBillboard::ShadowBillboard(SceneGraph* scene, const char* map) : Material(scene, -1)
{
   addTexture(_color_map, map, 1 | 2 | 4);
}

void ShadowBillboard::removeMesh(Mesh* mesh)
{
   for (int32_t i = 0; i < mesh->getPartCount(); i++)
   {
      _instances.erase(mesh->getPart(i));
   }
}

void ShadowBillboard::init()
{
   _shader = activeDevice->loadShader("shadowbillboard-vert.glsl", "shadowbillboard-frag.glsl");
   _param_texture = activeDevice->getParameterIndex("texturemap");

   _vertices = activeDevice->createVertexBuffer(sizeof(Vector) * 4 * max_billboards, true);
   _texcoords = activeDevice->createVertexBuffer(sizeof(float) * 2 * 4 * max_billboards);
   _indices = activeDevice->createIndexBuffer(sizeof(uint16_t) * 6 * max_billboards);

   volatile float* texcoord = static_cast<float*>(activeDevice->lockVertexBuffer(_texcoords));
   const float min_uv = 0.01f;
   const float max_uv = 0.99f;

   for (int32_t i = 0; i < max_billboards; i++)
   {
      *texcoord++ = min_uv;
      *texcoord++ = max_uv;

      *texcoord++ = max_uv;
      *texcoord++ = max_uv;

      *texcoord++ = max_uv;
      *texcoord++ = min_uv;

      *texcoord++ = min_uv;
      *texcoord++ = min_uv;
   }
   activeDevice->unlockVertexBuffer(_texcoords);

   volatile uint16_t* index = static_cast<uint16_t*>(activeDevice->lockIndexBuffer(_indices));
   for (int32_t i = 0; i < max_billboards; i++)
   {
      *index++ = static_cast<uint16_t>(i * 4 + 0);
      *index++ = static_cast<uint16_t>(i * 4 + 1);
      *index++ = static_cast<uint16_t>(i * 4 + 2);
      *index++ = static_cast<uint16_t>(i * 4 + 0);
      *index++ = static_cast<uint16_t>(i * 4 + 2);
      *index++ = static_cast<uint16_t>(i * 4 + 3);
   }
   activeDevice->unlockIndexBuffer(_indices);
}

void ShadowBillboard::load(Stream* stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0)->name());
}

void ShadowBillboard::setOffset(float x, float y)
{
   _offset.set(x, y);
}

void ShadowBillboard::addGeometry(Geometry* geometry)
{
   Bounding bound;
   bound.min = Vector(-1.5f - _offset.x, -1.5f - _offset.y);
   bound.max = Vector(1.5f - _offset.x, 1.5f - _offset.y);
   _instances[geometry] = bound;
}

void ShadowBillboard::begin()
{
   Material::begin();

   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ZERO, GL_SRC_COLOR);

   glActiveTexture(GL_TEXTURE0_ARB);
   glBindTexture(GL_TEXTURE_2D, _color_map);

   activeDevice->setShader(_shader);
   activeDevice->bindSampler(_param_texture, 0);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(2);
}

void ShadowBillboard::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(2);

   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   glDisable(GL_BLEND);
   glDepthMask(GL_TRUE);

   activeDevice->setShader(0);
}

void ShadowBillboard::renderDiffuse()
{
   begin();

   const float z = 0.1f;
   int32_t count = 0;
   // explicit size - lockVertexBuffer()'s size-less overload falls back to GLDevice's single
   // shared "last created buffer" size, which by this point in the frame belongs to whatever
   // other material most recently created a buffer, not this one. Silently mapped the wrong byte
   // range on every frame after the first, so this never rendered anything beyond one lucky frame.
   Vector* destination = static_cast<Vector*>(activeDevice->lockVertexBuffer(_vertices, sizeof(Vector) * 4 * max_billboards));
   if (!destination)
   {
      // glMapBufferRange can fail (buffer still mapped from a prior call, GL error pending,
      // etc.) - skip this frame's shadows rather than dereference a null pointer.
      end();
      return;
   }

   for (const auto& [geometry, bound] : _instances)
   {
      // the mapped vertex buffer holds max_billboards quads
      if (count >= max_billboards)
      {
         break;
      }

      if (geometry->isVisible())
      {
         const Matrix object = geometry->getTransform().normalized();

         const Vector bound_min = bound.min + object.translation();
         const Vector bound_max = bound.max + object.translation();

         destination->x = bound_min.x;
         destination->y = bound_min.y;
         destination->z = z;
         destination++;

         destination->x = bound_max.x;
         destination->y = bound_min.y;
         destination->z = z;
         destination++;

         destination->x = bound_max.x;
         destination->y = bound_max.y;
         destination->z = z;
         destination++;

         destination->x = bound_min.x;
         destination->y = bound_max.y;
         destination->z = z;
         destination++;

         count++;
      }
   }
   activeDevice->unlockVertexBuffer(_vertices);

   glBindBuffer(GL_ARRAY_BUFFER, _vertices);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector), nullptr);
   glBindBuffer(GL_ARRAY_BUFFER, _texcoords);
   glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, nullptr);

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indices);

   // vertices above are already baked in world space (bound.min/max + object.translation()), unlike
   // every other material here which pushes per-geometry transforms - so this is the one material
   // that needs an explicit identity push. Without it, this shader's u_modelViewProjection uniform
   // is never uploaded at all (push() is the only thing that uploads it), leaving it at GLSL's
   // zero-initialized default and collapsing every shadow vertex to the origin - invisible, even
   // though the draw call itself succeeds.
   activeDevice->push(Matrix());
   glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_SHORT, nullptr);
   activeDevice->pop();

   end();
}
