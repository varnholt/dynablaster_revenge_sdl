#pragma once

#include "math/matrix.h"
#include "tools/array.h"

class Node;

class BakedTransformation : public Array<Matrix>
{
public:
   BakedTransformation() = default;
   BakedTransformation(Node* node, float step_size);

   Matrix interpolate(float frame) const;

private:
   float _step_size = 0.0f;
};
