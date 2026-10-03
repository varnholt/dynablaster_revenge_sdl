#include "playermaterial.h"
#include <array>
#include "animation/motionmixer.h"
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

PlayerMaterial::PlayerMaterial() : PlayerMaterialBase(MAP_DIFFUSE | MAP_REFLECT)
{
}

PlayerMaterial::PlayerMaterial(
   const std::string& color_map,
   const std::string& environment_map,
   const std::string& specular_map,
   const std::string& ambient_occlusion_map
)
    : PlayerMaterialBase(MAP_DIFFUSE | MAP_REFLECT)
{
   addTexture(_color_map, color_map);
   addTexture(_diffuse_map, environment_map);
   addTexture(_specular_map, specular_map, 1 | 2 | 4);
   addTexture(_ambient_map, ambient_occlusion_map);
}

void PlayerMaterial::init()
{
   _shader = activeDevice().loadShader("playermaterial-vert.glsl", "playermaterial-frag.glsl");

   _param_specular = activeDevice().getParameterIndex("specularmap");
   _param_diffuse = activeDevice().getParameterIndex("diffusemap");
   _param_texture = activeDevice().getParameterIndex("texturemap");
   _param_ambient = activeDevice().getParameterIndex("ambientmap");
   _param_camera = activeDevice().getParameterIndex("camera");
   _param_bones = activeDevice().getParameterIndex("bones");
   _param_flash = activeDevice().getParameterIndex("flash");
   _param_fade = activeDevice().getParameterIndex("fade");
}

void PlayerMaterial::load(Stream& stream)
{
   Material::load(stream);

   addTexture(_color_map, getTextureSlot(0).name());
   addTexture(_diffuse_map, "diffuse_level");
   addTexture(_specular_map, getTextureSlot(1).name(), 1 | 2 | 4);
}

void PlayerMaterial::begin()
{
   Material::begin();

   glBindTexture(GL_TEXTURE_2D, _specular_map);

   glActiveTexture(GL_TEXTURE1_ARB);
   glBindTexture(GL_TEXTURE_2D, _diffuse_map);

   glActiveTexture(GL_TEXTURE2_ARB);
   glBindTexture(GL_TEXTURE_2D, _color_map);

   glActiveTexture(GL_TEXTURE3_ARB);
   glBindTexture(GL_TEXTURE_2D, _ambient_map);

   activeDevice().setShader(_shader);
   activeDevice().bindSampler(_param_specular, 0);
   activeDevice().bindSampler(_param_diffuse, 1);
   activeDevice().bindSampler(_param_texture, 2);
   activeDevice().bindSampler(_param_ambient, 3);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);

   // texcoord0 (location 2) plus two more vec4 attributes (3, 4) that the legacy renderer
   // smuggled through texture units 1/2's texcoord slots (skinning data - see renderDiffuse()).
   glEnableVertexAttribArray(2);
   glEnableVertexAttribArray(3);
   glEnableVertexAttribArray(4);
}

void PlayerMaterial::end()
{
   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(3);
   glDisableVertexAttribArray(4);

   glActiveTexture(GL_TEXTURE0);

   activeDevice().setShader(0);
}

void PlayerMaterial::renderDiffuse()
{
   begin();

   std::array<Matrix, max_cluster_bones> bones;
   for (const Buffer& buffer : _buffers)
   {
      const VertexBuffer& vertex_buffer = buffer.vertex_buffer;
      Geometry& geometry = buffer.geometry;

      if (geometry.isVisible())
      {
         const Mesh& mesh = geometry.getParent();
         const auto mixer = mesh.getMotionMixer();

         const float flash = mesh.getRenderParameter(0);
         const float fade = mesh.getRenderParameter(2);

         const Matrix inverse_view = (geometry.getTransform() * _camera).invert();
         const Vector object_space_camera = inverse_view.translation();

         activeDevice().setParameter(_param_flash, flash);
         activeDevice().setParameter(_param_fade, fade);
         activeDevice().setParameter(_param_camera, object_space_camera);

         activeDevice().push(geometry.getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer.getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));

         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2));
         glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2 + 2 * 4));
         glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2 + 6 * 4));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer.getIndexBuffer());

         // render all clusters from the same vertex buffer
         size_t index_offset = 0;
         int32_t end_vertex = 0;
         for (const auto& cluster : _clusters)
         {
            const int32_t bone_count = cluster->boneCount();
            for (int32_t i = 0; i < bone_count; i++)
            {
               bones[i] = mixer->get().getNode(cluster->bones[i]).getTransform();
            }
            for (int32_t i = bone_count; i < max_cluster_bones; i++)
            {
               bones[i] = Matrix();
            }
            glUniformMatrix4fv(_param_bones, max_cluster_bones, false, reinterpret_cast<const float*>(bones.data()));

            [[maybe_unused]] const int32_t start_vertex = end_vertex;  // unused on GLES (see gles3.h)
            end_vertex += static_cast<int32_t>(cluster->vertices.size());
            const auto index_count = static_cast<GLsizei>(cluster->indices.size());
            glDrawRangeElements(
               GL_TRIANGLES,
               start_vertex,
               end_vertex,
               index_count,
               GL_UNSIGNED_SHORT,
               reinterpret_cast<const void*>(index_offset * sizeof(uint16_t))
            );  // render
            index_offset += cluster->indices.size();
         }

         activeDevice().pop();
      }
   }

   end();
}

void PlayerMaterial::setColorMap(const Texture& color_map)
{
   _color_map = color_map;
}

void PlayerMaterial::exportOBJ(Stream& stream, int32_t& index_offset)
{
   for (const Buffer& buffer : _buffers)
   {
      Geometry& geometry = buffer.geometry;

      if (!geometry.isVisible())
      {
         continue;
      }

      const std::vector<Vector> vertices = geometry.getSkinVertices();

      exportGeo(
         stream,
         geometry.getParent().name(),
         geometry.getTransform(),
         vertices,
         geometry.getNormals(),
         geometry.getUV(1),
         geometry.getIndices(),
         index_offset
      );

      index_offset += geometry.getVertexCount();
   }
}
