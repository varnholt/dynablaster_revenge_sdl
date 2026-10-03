#pragma once

#include "math/matrix.h"
#include "tools/streamable.h"
#include "weight.h"

#include <cstdint>
#include <span>
#include <vector>

class Bone : public Streamable
{
public:
   Bone() = default;
   Bone(const Bone& bone) = default;
   virtual ~Bone() = default;

   Bone& operator=(const Bone& bone) = default;

   int32_t id() const;
   void setId(int32_t id);
   const Matrix& transform() const;
   const std::vector<Weight>& weightList() const;
   std::span<const Weight> weights() const;
   int32_t count() const;

   void load(Stream& stream) override;
   void write(Stream& stream) override;

private:
   int32_t _id = -1;
   Matrix _init_transform;
   std::vector<Weight> _weights;
};
