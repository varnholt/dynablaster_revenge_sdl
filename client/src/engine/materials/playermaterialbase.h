#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>
#include "material.h"
#include "math/matrix.h"
#include "render/uv.h"

class PlayerMaterialBase : public Material
{
public:
   // maximum number of bones influencing a single cluster (matches the shader's bone array)
   static constexpr int32_t max_cluster_bones = 8;

   // part of a skinned mesh that is influenced by at most max_cluster_bones bones
   struct Cluster
   {
      void mergeBones(const std::vector<int32_t>& list, std::vector<int32_t>& usages);
      void addVertexIndex(int32_t vertex_index);
      int32_t boneCount() const;
      bool containsBone(int32_t id) const;

      std::vector<int32_t> bones;
      std::vector<int32_t> vertices;
      std::vector<uint16_t> indices;
      std::vector<std::vector<float>> weights;  // per vertex, one weight per cluster bone
   };

   struct Vertex
   {
      Vector position;
      Vector normal;
      UV uv;
      std::array<float, max_cluster_bones> weight;
   };
   static_assert(sizeof(Vertex) == sizeof(Vector) * 2 + sizeof(UV) + sizeof(float) * max_cluster_bones);

   PlayerMaterialBase(int32_t id);

   void update(float frame, const Matrix& camera) override;
   void addGeometry(Geometry& geometry) override;

   void getBoundingRect(Vector& min, Vector& max, const Matrix& projection) override;
   Vector getCenter2d(const Matrix& projection) const override;

private:
   std::vector<std::unique_ptr<Cluster>> createSkinClusters(const Geometry& geometry, int32_t limit);

protected:
   // shared by all player materials: every player uses the same skinned mesh
   static std::vector<std::unique_ptr<Cluster>> _clusters;
   Matrix _camera;
};
