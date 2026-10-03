#include "bakedtransformation.h"
#include <cmath>
#include <cstdint>
#include "nodes/node.h"

BakedTransformation::BakedTransformation(Node& node, float step_size) : _step_size(1.0f / step_size)
{
   const int32_t max_frame = node.getAnimationLength();
   int32_t keys = static_cast<int32_t>(std::ceil(max_frame / step_size));
   if (keys == 0)
   {
      keys = 1;
   }

   auto matrices = std::make_shared<std::vector<Matrix>>();
   matrices->reserve(keys);

   for (int32_t i = 0; i < keys; i++)
   {
      node.transform(i * step_size);
      matrices->push_back(node.getTransform());
   }

   _matrices = std::move(matrices);
}

int32_t BakedTransformation::size() const
{
   return _matrices ? static_cast<int32_t>(_matrices->size()) : 0;
}

Matrix BakedTransformation::interpolate(float frame) const
{
   const int32_t count = size();

   if (frame <= 0.0f)
   {
      return (*_matrices)[0];
   }

   frame *= _step_size;
   const int32_t index = static_cast<int32_t>(std::floor(frame));

   // make sure [index] and [index+1] exist for blending
   if (index < count - 1)
   {
      const float t = 1.0f - (index + 1 - frame);
      const Matrix& m1 = (*_matrices)[index];
      const Matrix& m2 = (*_matrices)[index + 1];
      return Matrix::blend(m1, m2, t);
   }

   // last index
   if (count > 0)
   {
      return _matrices->back();
   }

   // track is empty: identity
   return Matrix();
}
