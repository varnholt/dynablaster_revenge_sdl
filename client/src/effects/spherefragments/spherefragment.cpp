#include "spherefragment.h"

// engine
#include "gldevice.h"
#include "image/image.h"
#include "math/vector.h"
#include "nodes/mesh.h"
#include "render/geometry.h"
#include "render/uv.h"
#include "tools/random.h"
#include "vertex3d.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

namespace
{
constexpr float PI = std::numbers::pi_v<float>;
}  // namespace

SphereFragment::SphereFragment(const std::vector<Mesh*>& meshes, const Image* order_image)
{
   _matrix.reserve(meshes.size());

   for (const Mesh* mesh : meshes)
   {
      const Matrix& matrix = mesh->getTransform();
      const Geometry* geometry = mesh->getPart(0);

      Vector p = matrix.translation();
      p.normalize();
      p += Vector(0, 0, 1);

      const float m = 0.5f / std::sqrt(p * p);
      const float u = 0.5f + p.x * m;
      const float v = 0.5f + p.y * m;

      const uint32_t rgb = order_image->getPixel(u, v);
      const float order = ((rgb >> 16 & 255) + (rgb >> 8 & 255) + (rgb & 255)) / 96.0f;

      _vertex_count += geometry->getVertexCount();
      _index_count += geometry->getIndexCount();
      _matrix.push_back(matrix);

      _random.push_back(frand(1.0f));
      _model_view.emplace_back();
      _fresnel_factors.push_back(1.0f);
      _time.push_back(order);
   }

   // create vertex and index buffer
   _vertex_buffer = activeDevice->createVertexBuffer(_vertex_count * sizeof(Vertex3D));

   // fill vertex buffer
   auto* vertex = static_cast<Vertex3D*>(activeDevice->lockVertexBuffer(_vertex_buffer));

   for (size_t m = 0; m < meshes.size(); m++)
   {
      const Mesh* mesh = meshes[m];
      const Matrix& matrix = mesh->getTransform();
      const Matrix normal_matrix = matrix.get3x3();
      const Geometry* geometry = mesh->getPart(0);
      const Vector* vertices = geometry->getVertices();
      const Vector* normals = geometry->getNormals();
      const UV* uvs = geometry->getUV(1);

      for (int32_t v = 0; v < geometry->getVertexCount(); v++)
      {
         const Vector& p = vertices[v];
         const Vector& n = normals[v];

         Vector surface = matrix * p;
         const Vector normal = normal_matrix * n;
         surface.normalize();

         const float orientation = std::max(normal * surface, 0.0f);

         vertex->position.x = p.x;
         vertex->position.y = p.y;
         vertex->position.z = p.z;
         vertex->normal.x = n.x;
         vertex->normal.y = n.y;
         vertex->normal.z = n.z;
         vertex->u = uvs[v].u;
         vertex->v = uvs[v].v;
         vertex->index = static_cast<float>(m);
         vertex->blend = orientation * orientation;
         vertex->tangent.x = -n.y;
         vertex->tangent.y = n.x;
         vertex->tangent.z = 0.0f;
         vertex++;
      }
   }

   activeDevice->unlockVertexBuffer(_vertex_buffer);

   // fill index buffer
   _index_buffer = activeDevice->createIndexBuffer(_index_count * sizeof(uint16_t));
   auto* index = static_cast<uint16_t*>(activeDevice->lockIndexBuffer(_index_buffer));
   uint16_t offset = 0;
   for (const Mesh* mesh : meshes)
   {
      const Geometry* geometry = mesh->getPart(0);
      const uint16_t* indices = geometry->getIndices();
      for (int32_t i = 0; i < geometry->getIndexCount(); i++)
      {
         *index++ = offset + indices[i];
      }
      offset += geometry->getVertexCount();
   }
   activeDevice->unlockIndexBuffer(_index_buffer);
}

int32_t SphereFragment::getPartCount() const
{
   return static_cast<int32_t>(_matrix.size());
}

const Matrix* SphereFragment::getMatrices() const
{
   return _model_view.data();
}

float* SphereFragment::getFresnelFactors()
{
   return _fresnel_factors.data();
}

void SphereFragment::animate(float time, const Matrix& rotation)
{
   for (size_t i = 0; i < _matrix.size(); i++)
   {
      const Matrix& matrix = _matrix[i];
      const Vector position = matrix.translation();
      Matrix fragment_matrix = matrix.get3x3();

      // 4 * PI
      // => one loop of action
      // => another loop of idle
      const float t = std::min(std::fmod(time * 2.5f + _time[i], PI * 4.0f), PI * 2.0f);

      const float time_growth = std::sin(t + 1.5f * PI);
      const float position_scale = 1.0f + _random[i] * (0.1f * (1.0f + time_growth));

      // scale down during extension
      float size = 0.5f * (1.0f + time_growth);
      size = 1.0f - 0.2f * size;
      size *= size;

      // 0..1
      const float amount = std::sin(t * 0.5f - PI * 0.5f) * 0.5f + 0.5f;

      // 0..2pi
      const float r = amount * PI * 2.0f;
      const Matrix fragment_rotation = Matrix::scale(size, size, size) * Matrix::rotateX(r) * Matrix::rotateY(r);

      fragment_matrix.translate(position * position_scale);

      _model_view[i] = fragment_rotation * fragment_matrix * rotation;

      _fresnel_factors[i] = 1.0f - std::sin(t * 0.5f);
   }
}

void SphereFragment::draw()
{
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glEnableVertexAttribArray(2);
   glEnableVertexAttribArray(3);

   glBindBuffer(GL_ARRAY_BUFFER, _vertex_buffer);
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), nullptr);
   glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<const void*>(offsetof(Vertex3D, normal)));
   glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<const void*>(offsetof(Vertex3D, u)));
   glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex3D), reinterpret_cast<const void*>(offsetof(Vertex3D, tangent)));

   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _index_buffer);
   glDrawElements(GL_TRIANGLES, _index_count, GL_UNSIGNED_SHORT, nullptr);

   glDisableVertexAttribArray(3);
   glDisableVertexAttribArray(2);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(0);
}
