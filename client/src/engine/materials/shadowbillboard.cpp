#include "shadowbillboard.h"
#include <algorithm>
#include <array>
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/geometry.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

ShadowBillboard::ShadowBillboard() : Material(-1)
{
}

ShadowBillboard::ShadowBillboard(const std::string& map) : Material(-1)
{
   addTexture(_color_map, map, 1 | 2 | 4);
}

void ShadowBillboard::removeMesh(const Mesh& mesh)
{
   std::erase_if(_instances, [&mesh](const Instance& instance) { return &instance.geometry.get().getParent() == &mesh; });
}

void ShadowBillboard::init()
{
   _shader = activeDevice().loadShader("shadowbillboard-vert.glsl", "shadowbillboard-frag.glsl");
   _param_texture = activeDevice().getParameterIndex("texturemap");

   _vertices = activeDevice().createVertexBuffer(sizeof(Vector) * 4 * max_billboards, true);
   _texcoords = activeDevice().createVertexBuffer(sizeof(float) * 2 * 4 * max_billboards);
   _indices = activeDevice().createIndexBuffer(sizeof(uint16_t) * 6 * max_billboards);

   const std::span<float> texcoords = activeDevice().lockVertexBuffer<float>(_texcoords);
   constexpr float min_uv = 0.01f;
   constexpr float max_uv = 0.99f;
   constexpr std::array<float, 8> quad_uvs = {min_uv, max_uv, max_uv, max_uv, max_uv, min_uv, min_uv, min_uv};

   for (int32_t i = 0; i < max_billboards; i++)
   {
      std::ranges::copy(quad_uvs, texcoords.subspan(i * quad_uvs.size()).begin());
   }
   activeDevice().unlockVertexBuffer(_texcoords);

   const std::span<uint16_t> indices = activeDevice().lockIndexBuffer<uint16_t>(_indices);
   constexpr std::array<int32_t, 6> quad_indices = {0, 1, 2, 0, 2, 3};
   for (int32_t i = 0; i < max_billboards; i++)
   {
      for (size_t corner = 0; corner < quad_indices.size(); corner++)
      {
         indices[i * quad_indices.size() + corner] = static_cast<uint16_t>(i * 4 + quad_indices[corner]);
      }
   }
   activeDevice().unlockIndexBuffer(_indices);
}

void ShadowBillboard::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0).name());
}

void ShadowBillboard::setOffset(float x, float y)
{
   _offset.set(x, y);
}

void ShadowBillboard::addGeometry(Geometry& geometry)
{
   Bounding bound;
   bound.min = Vector(-1.5f - _offset.x, -1.5f - _offset.y);
   bound.max = Vector(1.5f - _offset.x, 1.5f - _offset.y);
   const auto instance =
      std::ranges::find_if(_instances, [&geometry](const Instance& candidate) { return &candidate.geometry.get() == &geometry; });
   if (instance != _instances.end())
   {
      instance->bound = bound;
   }
   else
   {
      _instances.push_back({geometry, bound});
   }
}

void ShadowBillboard::begin()
{
   Material::begin();

   glDepthMask(GL_FALSE);
   glEnable(GL_BLEND);
   glBlendFunc(GL_ZERO, GL_SRC_COLOR);

   glActiveTexture(GL_TEXTURE0_ARB);
   glBindTexture(GL_TEXTURE_2D, _color_map);

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_param_texture, 0);

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

   activeDevice().setShader(0);
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
   const std::span<Vector> destination = activeDevice().lockVertexBuffer<Vector>(_vertices, sizeof(Vector) * 4 * max_billboards);
   if (destination.empty())
   {
      // glMapBufferRange can fail (buffer still mapped from a prior call, GL error pending,
      // etc.) - skip this frame's shadows rather than dereference a null pointer.
      end();
      return;
   }

   for (const auto& [geometry_ref, bound] : _instances)
   {
      const Geometry& geometry = geometry_ref;

      // the mapped vertex buffer holds max_billboards quads
      if (count >= max_billboards)
      {
         break;
      }

      if (geometry.isVisible())
      {
         const Matrix object = geometry.getTransform().normalized();

         const Vector bound_min = bound.min + object.translation();
         const Vector bound_max = bound.max + object.translation();

         const std::span<Vector> quad = destination.subspan(count * 4, 4);
         quad[0] = Vector(bound_min.x, bound_min.y, z);
         quad[1] = Vector(bound_max.x, bound_min.y, z);
         quad[2] = Vector(bound_max.x, bound_max.y, z);
         quad[3] = Vector(bound_min.x, bound_max.y, z);

         count++;
      }
   }
   activeDevice().unlockVertexBuffer(_vertices);

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
   activeDevice().push(Matrix());
   glDrawElements(GL_TRIANGLES, count * 6, GL_UNSIGNED_SHORT, nullptr);
   activeDevice().pop();

   end();
}
