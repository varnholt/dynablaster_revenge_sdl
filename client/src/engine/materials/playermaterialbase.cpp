#include "playermaterialbase.h"
#include <algorithm>
#include <iterator>
#include "animation/motionmixer.h"
#include "gldevice.h"
#include "nodes/mesh.h"
#include "render/renderbuffer.h"
#include "render/texturepool.h"
#include "render/uv.h"
#include "render/vertexbuffer.h"
#include "tools/stream.h"

std::vector<std::unique_ptr<PlayerMaterialBase::Cluster>> PlayerMaterialBase::_clusters;

Vector PlayerMaterialBase::getCenter2d(const Matrix& projection) const
{
   Vector center(0.0f);

   Geometry* geometry = getGeometry(0);
   if (geometry)
   {
      const Node* mesh = geometry->getParent();
      const Matrix matrix = mesh->getTransform() * projection;

      const float t = 1.0f / matrix.ww;
      center.x = matrix.xw * t;
      center.y = matrix.yw * t;
      center.z = 0.0f;
   }
   return center;
}

void PlayerMaterialBase::getBoundingRect(Vector& min, Vector& max, const Matrix& projection)
{
   // realistic world space coordinates are somehwere around 0..13
   min = Vector(10000.0f, 10000.0f, 0.0f);
   max = Vector(-10000.0f, -10000.0f, 0.0f);

   for (int32_t j = 0; j < geometryCount(); j++)
   {
      Geometry* geometry = getGeometry(j);
      const Node* mesh = geometry->getParent();

      const Matrix matrix = mesh->getTransform() * projection;

      // get bounding box
      const Array<Vector> vertices = geometry->getSkinVertices();
      const int32_t vertex_count = geometry->getVertexCount();
      for (int32_t i = 0; i < vertex_count; i++)
      {
         const Vector& v = vertices[i];
         float x = matrix.xx * v.x + matrix.xy * v.y + matrix.xz * v.z + matrix.xw;
         float y = matrix.yx * v.x + matrix.yy * v.y + matrix.yz * v.z + matrix.yw;
         const float w = matrix.wx * v.x + matrix.wy * v.y + matrix.wz * v.z + matrix.ww;

         const float t = 1.0f / w;
         x *= t;
         y *= t;

         max.maximum(Vector(x, y, 0.0f));
         min.minimum(Vector(x, y, 0.0f));
      }
   }
}

void PlayerMaterialBase::Cluster::mergeBones(const std::vector<int32_t>& list, std::vector<int32_t>& usages)
{
   for (const int32_t bone : list)
   {
      usages[bone]--;
      if (!containsBone(bone))
      {
         bones.push_back(bone);
      }
   }
}

void PlayerMaterialBase::Cluster::addVertexIndex(int32_t vertex_index)
{
   const auto vertex = std::ranges::find(vertices, vertex_index);
   int32_t index = static_cast<int32_t>(std::distance(vertices.begin(), vertex));
   if (vertex == vertices.end())
   {
      vertices.push_back(vertex_index);
   }
   indices.push_back(static_cast<uint16_t>(index));
}

int32_t PlayerMaterialBase::Cluster::boneCount() const
{
   return static_cast<int32_t>(bones.size());
}

bool PlayerMaterialBase::Cluster::containsBone(int32_t id) const
{
   return std::ranges::find(bones, id) != bones.end();
}

PlayerMaterialBase::PlayerMaterialBase(SceneGraph* scene, int32_t id) : Material(scene, id)
{
}

std::vector<std::unique_ptr<PlayerMaterialBase::Cluster>> PlayerMaterialBase::createSkinClusters(Geometry* geometry, int32_t limit)
{
   std::vector<std::unique_ptr<Cluster>> clusters;

   FaceList faces;
   faces.copy(geometry->getIndicesList());

   // original data set is
   // bone 1: [vertex1, w1], [vertex2, w2], ...
   // bone 2: [vertex1, w1], [vertex2, w2], ...

   // create reverse order:
   // vertex1: [bone1, w1], [bone2, w2] ...
   // vertex2: [bone1, w1], [bone2, w2] ...

   std::vector<std::vector<int32_t>> vertex_bones(geometry->getVertexCount());
   std::vector<std::vector<float>> vertex_weights(geometry->getVertexCount());
   std::vector<int32_t> bone_usage(geometry->getBoneCount());
   for (int32_t b = 0; b < geometry->getBoneCount(); b++)
   {
      const Bone& bone = geometry->getBone(b);
      const Weight* weights = bone.weights();
      bone_usage[b] = bone.count();
      for (int32_t v = 0; v < bone.count(); v++)
      {
         const int32_t vertex = weights[v].id();
         vertex_bones[vertex].push_back(bone.id());
         vertex_weights[vertex].push_back(weights[v].weight());
      }
   }

   std::unique_ptr<Cluster> cluster;
   while (faces.size() > 0)
   {
      if (!cluster)
      {
         cluster = std::make_unique<Cluster>();
      }

      // find the triangle with the best match of vertex weights
      int32_t best_match = 0;  // maximize number of matching bones
      int32_t best_new = 100;  // minimize number of new bones
      int32_t best_triangle = -1;
      int32_t best_use = 10000;
      for (int32_t i = 0; i < faces.size(); i += 3)
      {
         int32_t current_match = 0;
         int32_t current_new = 0;
         int32_t current_use = 1000;
         for (int32_t corner = 0; corner < 3; corner++)
         {
            for (const int32_t bone : vertex_bones[faces[i + corner]])
            {
               if (cluster->containsBone(bone))
               {
                  current_match++;
               }
               else
               {
                  current_new++;
                  current_use = std::min(current_use, bone_usage[bone]);
               }
            }
         }

         if (current_new <= best_new)
         {
            if (current_new < best_new || current_match >= best_match || current_use < best_use)
            {
               best_triangle = i;
               best_new = current_new;
               best_match = current_match;
               best_use = current_use;
            }
         }
      }

      if (cluster->boneCount() + best_new <= limit)
      {
         // add triangle to cluster
         for (int32_t corner = 0; corner < 3; corner++)
         {
            const int32_t vertex = faces[best_triangle];
            cluster->mergeBones(vertex_bones[vertex], bone_usage);

            cluster->addVertexIndex(vertex);
            faces.erase(best_triangle);  // index "best_triangle+1" becomes "best_triangle"
         }
      }
      else
      {
         // no triangle fits into the given maximum of bone influences: start a new cluster
         clusters.push_back(std::move(cluster));
      }
   }

   if (cluster)
   {
      clusters.push_back(std::move(cluster));
   }

   // figure weights for remapped vertices
   for (const auto& current : clusters)
   {
      for (const int32_t vertex : current->vertices)
      {
         const std::vector<int32_t>& bones_of_vertex = vertex_bones[vertex];
         const std::vector<float>& weights_of_vertex = vertex_weights[vertex];

         // build shared bone/weight list, unused bones get zero weight
         std::vector<float> weights(limit, 0.0f);
         for (int32_t b = 0; b < current->boneCount(); b++)
         {
            const auto bone = std::ranges::find(bones_of_vertex, current->bones[b]);
            if (bone != bones_of_vertex.end())
            {
               weights[b] = weights_of_vertex[std::distance(bones_of_vertex.begin(), bone)];
            }
         }

         current->weights.push_back(std::move(weights));
      }
   }

   return clusters;
}

void PlayerMaterialBase::addGeometry(Geometry* geometry)
{
   VertexBuffer* vertex_buffer = _pool->get(geometry);
   if (!vertex_buffer)
   {
      vertex_buffer = _pool->add(geometry);

      if (_clusters.empty())
      {
         _clusters = createSkinClusters(geometry, max_cluster_bones);
      }

      // get total number of vertices from clusters
      int32_t total_vertices = 0;
      int32_t total_indices = 0;
      for (const auto& cluster : _clusters)
      {
         total_vertices += static_cast<int32_t>(cluster->vertices.size());
         total_indices += static_cast<int32_t>(cluster->indices.size());
      }

      activeDevice->allocateVertexBuffer(vertex_buffer->getVertexBuffer(), sizeof(Vertex) * total_vertices);
      {
         volatile Vertex* destination = static_cast<Vertex*>(activeDevice->lockVertexBuffer(vertex_buffer->getVertexBuffer()));

         const Vector* vertices = geometry->getVertices();
         const Vector* normals = geometry->getNormals();
         const UV* uv = geometry->getUV(1);

         for (const auto& cluster : _clusters)
         {
            for (size_t i = 0; i < cluster->vertices.size(); i++)
            {
               const int32_t index = cluster->vertices[i];
               destination->position.x = vertices[index].x;
               destination->position.y = vertices[index].y;
               destination->position.z = vertices[index].z;

               destination->normal.x = normals[index].x;
               destination->normal.y = normals[index].y;
               destination->normal.z = normals[index].z;

               destination->uv.u = uv[index].u;
               destination->uv.v = uv[index].v;

               for (int32_t j = 0; j < max_cluster_bones; j++)
               {
                  destination->weight[j] = cluster->weights[i][j];
               }
               destination++;
            }
         }
         activeDevice->unlockVertexBuffer(vertex_buffer->getVertexBuffer());
      }

      {
         activeDevice->allocateIndexBuffer(vertex_buffer->getIndexBuffer(), total_indices * sizeof(uint16_t));
         volatile uint16_t* destination = static_cast<uint16_t*>(activeDevice->lockIndexBuffer(vertex_buffer->getIndexBuffer()));
         int32_t offset = 0;
         for (const auto& cluster : _clusters)
         {
            for (const uint16_t index : cluster->indices)
            {
               *destination++ = static_cast<uint16_t>(index + offset);
            }
            offset += static_cast<int32_t>(cluster->vertices.size());
         }
         activeDevice->unlockIndexBuffer(vertex_buffer->getIndexBuffer());
      }

      vertex_buffer->setIndexCount(total_indices);
   }
   _buffers.push_back({geometry, vertex_buffer});
}

void PlayerMaterialBase::update(float /*frame*/, Node** /*node_list*/, const Matrix& camera)
{
   _camera = camera;
}
