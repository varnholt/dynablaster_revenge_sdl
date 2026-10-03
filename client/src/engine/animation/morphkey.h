// contains a morph-target for vertex-animations

#pragma once

#include <cstdint>
#include <vector>
#include "key.h"
#include "math/vector.h"

class MorphKey : public KeyBase
{
public:
   MorphKey() = default;

   const std::vector<Vector>& getVertices() const;
   const std::vector<Vector>& getNormals() const;

   int32_t getVertexCount() const;

   void load(Stream& stream) override;
   void write(Stream& stream) override;

   void calculateNormals(const std::vector<uint16_t>& index_buffer);

protected:
   std::vector<Vector> _vertices;
   std::vector<Vector> _normals;
};
