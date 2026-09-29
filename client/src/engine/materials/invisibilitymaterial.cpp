#include "invisibilitymaterial.h"
#include <array>
#include "animation/motionmixer.h"
#include "framework/globaltime.h"
#include "gldevice.h"
#include "image/image.h"
#include "nodes/mesh.h"
#include "render/renderbuffer.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "textureslot.h"
#include "tools/stream.h"

InvisibilityMaterial::InvisibilityMaterial(SceneGraph* scene) : PlayerMaterialBase(scene, MAP_DIFFUSE | MAP_REFLECT)
{
   addTexture(_gradient_map, "invisble-mask");
}

void InvisibilityMaterial::init()
{
   _shader = activeDevice->loadShader("invisibility-vert.glsl", "invisibility-frag.glsl");

   _param_texture = activeDevice->getParameterIndex("texturemap");
   _param_gradient = activeDevice->getParameterIndex("gradientmap");
   _param_fade_threshold = activeDevice->getParameterIndex("fadeThreshold");

   _param_camera = activeDevice->getParameterIndex("camera");
   _param_bones = activeDevice->getParameterIndex("bones");
}

void InvisibilityMaterial::load(Stream* stream)
{
   Material::load(stream);
}

void InvisibilityMaterial::begin()
{
   Material::begin();

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _texture_map);
   activeDevice->bindSampler(_param_texture, 0);

   glActiveTexture(GL_TEXTURE1);
   glBindTexture(GL_TEXTURE_2D, _gradient_map);
   activeDevice->bindSampler(_param_gradient, 1);

   activeDevice->setShader(_shader);

   // enable required vertex arrays
   glEnableVertexAttribArray(0);  // vertex data
   glEnableVertexAttribArray(1);

   // texcoord0 (location 2) plus two more vec4 attributes (3, 4) that the legacy renderer
   // smuggled through texture units 1/2's texcoord slots (skinning data - see renderDiffuse()).
   glEnableVertexAttribArray(2);
   glEnableVertexAttribArray(3);
   glEnableVertexAttribArray(4);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void InvisibilityMaterial::end()
{
   activeDevice->setShader(0);

   glDisableVertexAttribArray(0);  // vertex data
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(3);
   glDisableVertexAttribArray(4);

   glActiveTexture(GL_TEXTURE0);

   glDisable(GL_BLEND);
}

void InvisibilityMaterial::renderDiffuse()
{
   begin();

   std::array<Matrix, max_cluster_bones> bones;
   for (const Buffer& buffer : _buffers)
   {
      VertexBuffer* vertex_buffer = buffer.vertex_buffer;
      Geometry* geometry = buffer.geometry;

      if (geometry->isVisible())
      {
         const Mesh* mesh = static_cast<const Mesh*>(geometry->getParent());
         const float time = mesh->getRenderParameter(1);
         activeDevice->setParameter(_param_fade_threshold, time);

         const MotionMixer* mixer = mesh->getMotionMixer();

         const Matrix inverse_view = (geometry->getTransform() * _camera).invert();
         const Vector object_space_camera = inverse_view.translation();

         activeDevice->setParameter(_param_camera, object_space_camera);

         activeDevice->push(geometry->getTransform());

         // draw mesh
         glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer->getVertexBuffer());
         glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
         glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector)));

         glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2));
         glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2 + 2 * 4));
         glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const GLvoid*>(sizeof(Vector) * 2 + 6 * 4));

         glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertex_buffer->getIndexBuffer());

         // render all clusters from the same vertex buffer
         size_t index_offset = 0;
         int32_t end_vertex = 0;
         for (const auto& cluster : _clusters)
         {
            const int32_t bone_count = cluster->boneCount();
            for (int32_t i = 0; i < bone_count; i++)
            {
               bones[i] = mixer->getNode(cluster->bones[i])->getTransform();
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

         activeDevice->pop();
      }
   }

   end();
}

void InvisibilityMaterial::setTexture(uint32_t texture)
{
   _texture_map = texture;
}
