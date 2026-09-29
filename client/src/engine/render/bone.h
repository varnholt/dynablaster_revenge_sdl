#pragma once

#include "math/matrix.h"
#include "tools/array.h"
#include "tools/list.h"
#include "tools/streamable.h"
#include "weight.h"

#include <cstdint>

class Bone : public Streamable
{
public:
   Bone() = default;
   Bone(const Bone& bone) = default;
   Bone(const Bone& bone, Array<int32_t>* remap);
   virtual ~Bone() = default;

   Bone& operator=(const Bone& bone) = default;

   int32_t id() const;
   void setId(int32_t id);
   const Matrix& transform() const;
   const List<Weight>& weightList() const;
   Weight* weights() const;
   int32_t count() const;

   void load(Stream* stream) override;
   void write(Stream* stream) override;

private:
   int32_t _id = -1;
   Matrix _init_transform;
   List<Weight> _weights;
};
