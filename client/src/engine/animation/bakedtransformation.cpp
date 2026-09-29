#include "bakedtransformation.h"
#include <cmath>
#include <cstdint>
#include "nodes/node.h"

BakedTransformation::BakedTransformation(Node* node, float step_size) : _step_size(1.0f / step_size)
{
   const int32_t max_frame = node->getAnimationLength();
   int32_t keys = static_cast<int32_t>(std::ceil(max_frame / step_size));
   if (keys == 0)
   {
      keys = 1;
   }

   this->init(keys);

   for (int32_t i = 0; i < keys; i++)
   {
      node->transform(i * step_size);
      this->add(node->getTransform());
   }
}

Matrix BakedTransformation::interpolate(float frame) const
{
   if (frame <= 0.0f)
   {
      return this->get(0);
   }

   frame *= _step_size;
   const int32_t index = static_cast<int32_t>(std::floor(frame));

   // make sure [index] and [index+1] exist for blending
   if (index < this->size() - 1)
   {
      const float t = 1.0f - (index + 1 - frame);
      const Matrix& m1 = this->get(index);
      const Matrix& m2 = this->get(index + 1);
      return Matrix::blend(m1, m2, t);
   }

   // last index
   if (this->size() > 0)
   {
      return this->getLast();
   }

   // track is empty: identity
   return Matrix();
}
