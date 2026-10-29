#include "destructionmaterial.h"
#include "blockmaterial.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "animation/motionmixer.h"
#include "framework/gldevice.h"
#include "image/image.h"
#include "nodes/camera.h"
#include "nodes/mesh.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

DestructionMaterial::DestructionMaterial() : Material(MAP_DIFFUSE | MAP_REFLECT)
{
}

void DestructionMaterial::setColorMap(const std::string& color_map)
{
   replaceTexture(_color_map, color_map);
}

void DestructionMaterial::setGlass(std::vector<Vector> tints)
{
   _glass_tints = std::move(tints);
}

DestructionMaterial::DestructionMaterial(
   const std::string& color_map,
   const std::string& environment_map,
   const std::string& specular_map,
   const std::string& shadow_map,
   Camera& shadow_camera
)
    : Material(MAP_DIFFUSE | MAP_REFLECT), _shadow_camera(shadow_camera)
{
   addTexture(_color_map, color_map);
   addTexture(_diffuse_map, environment_map);
   addTexture(_specular_map, specular_map);
   addTexture(_shadow_map, shadow_map);
}

void DestructionMaterial::init()
{
   _shader = activeDevice().loadShader("destruction-vert.glsl", "destruction-frag.glsl");

   _param_specular = activeDevice().getParameterIndex("specularmap");
   _param_diffuse = activeDevice().getParameterIndex("diffusemap");
   _param_texture = activeDevice().getParameterIndex("texturemap");
   _param_shadow = activeDevice().getParameterIndex("shadowmap");
   _param_shadow_camera = activeDevice().getParameterIndex("shadowCamera");
   _param_glass = activeDevice().getParameterIndex("glass");
   _param_tint = activeDevice().getParameterIndex("tint");
}

void DestructionMaterial::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0).name());
   addTexture(_diffuse_map, "diffuse_level");
   addTexture(_specular_map, getTextureSlot(1).name());
}

void DestructionMaterial::addGeometry(Geometry& geometry)
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

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_param_specular, 0);
   activeDevice().bindSampler(_param_diffuse, 1);
   activeDevice().bindSampler(_param_texture, 2);
   activeDevice().bindSampler(_param_shadow, 3);
   activeDevice().setParameter(_param_glass, _glass_tints.empty() ? 0.0f : 1.0f);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);

   Matrix camera;
   if (_shadow_camera)
   {
      auto& device = static_cast<GLDevice&>(activeDevice());
      device.pushProjection();

      camera = _shadow_camera->get().getTransform().getView();
      float fov = _shadow_camera->get().getFOV();
      fov = std::tan(fov * 0.5) * 0.75;
      const float z_near = _shadow_camera->get().getNear();
      const float z_far = _shadow_camera->get().getFar();

      activeDevice().setCamera(camera, fov, z_near, z_far, _shadow_camera->get().getPerspectiveMode());

      camera = device.getProjectionMatrix();
      camera.normalizeZ();
      device.popProjection();
   }
   activeDevice().setParameter(_param_shadow_camera, camera);
}

void DestructionMaterial::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);

   glActiveTexture(GL_TEXTURE3_ARB);

   glActiveTexture(GL_TEXTURE0);

   activeDevice().setShader(0);
}

void DestructionMaterial::renderDiffuse()
{
   begin();

   for (const Buffer& buffer : _buffers)
   {
      const VertexBuffer& vertex_buffer = buffer.vertex_buffer;
      Geometry& geometry = buffer.geometry;

      if (geometry.isVisible())
      {
         if (!_glass_tints.empty())
         {
            // the debris hangs under a node standing where its block stood: the block's tint
            const auto block = geometry.getParent().parent();
            const Matrix& at = block ? block->get().getTransform() : geometry.getTransform();
            activeDevice().setParameter(_param_tint, _glass_tints[BlockMaterial::glassTint(at) % _glass_tints.size()]);
         }

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

   if (!_glass_tints.empty())
   {
      drawBreakingLight();
   }
}

void DestructionMaterial::drawBreakingLight()
{
   // once per broken block (its shards share the node standing where it stood), timed by its shards' frame
   std::vector<const Node*> done;
   BlockMaterial::beginLightPools();

   for (const Buffer& buffer : _buffers)
   {
      const Geometry& geometry = buffer.geometry;
      const Mesh& shard = geometry.getParent();
      const auto block = shard.parent();
      if (!geometry.isVisible() || !block || std::ranges::contains(done, &block->get()))
      {
         continue;
      }
      done.push_back(&block->get());

      // the debris' frames run 30 a game tick, 62.5 ticks a second (GameDrawable::animate())
      const float seconds = shard.getFrame() / (30.0f * 62.5f);
      if (seconds > 1.0f)
      {
         continue;
      }

      const Matrix& at = block->get().getTransform();
      const Vector& tint = _glass_tints[BlockMaterial::glassTint(at) % _glass_tints.size()];
      // the flash: bright and wide at once, spreading, gone within half a second
      BlockMaterial::drawLightPool(at, tint, 3.0f * std::exp(-seconds / 0.22f), 1.8f + seconds * 3.0f);
      // the block's own light lingering, fading out as the shards fly
      BlockMaterial::drawLightPool(at, tint, 0.7f * std::max(0.0f, 1.0f - seconds / 0.9f), 1.25f);
   }

   BlockMaterial::endLightPools();
}

void DestructionMaterial::update(float /*frame*/, const Matrix&)
{
}
