// contains a morph-target for vertex-animations

#pragma once

#include <cstdint>
#include "key.h"
#include "math/vector.h"
#include "tools/list.h"

class MorphKey : public KeyBase
{
public:
   MorphKey() = default;

   const List<Vector>& getVertices() const;
   const List<Vector>& getNormals() const;

   int32_t getVertexCount() const;

   void load(Stream* stream) override;
   void write(Stream* stream) override;

   void calculateNormals(const Array<uint16_t>& index_buffer);

protected:
   List<Vector> _vertices;
   List<Vector> _normals;
};
