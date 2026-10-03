#pragma once

#include "math/matrix.h"

#include <cstdint>
#include <memory>
#include <vector>

class Node;

// copies share the baked matrices
class BakedTransformation
{
public:
   BakedTransformation() = default;
   BakedTransformation(Node* node, float step_size);

   Matrix interpolate(float frame) const;
   int32_t size() const;

private:
   std::shared_ptr<const std::vector<Matrix>> _matrices;
   float _step_size = 0.0f;
};
